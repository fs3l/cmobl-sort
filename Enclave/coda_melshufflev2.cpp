/*
 * Author: Ju Chen
 * 10/24/2017
 */

#include <sgx_trts.h>
#include "./Enclave.h"
#include "./Enclave_t.h"
#include "./utility.h"
#include "./RealLibCoda.h"
#include "./multiqueue.hpp"
int32_t msortMem[2 * SqrtN * BLOWUPFACTOR];
int32_t tmp_data[2 * SqrtN * BLOWUPFACTOR];
int32_t g_interm[2 * BLOWUPFACTOR * N];
struct melshuffle_element {
  int32_t value;
  int32_t perm;
};
typedef struct melshuffle_element melshuffle_element_t;
#define QUEUE_CAP 10000

void c_merge(int32_t* dst, int32_t* src1, int32_t* src2, int stride)
{
  // memsetup
  int i = 0;
  int j = 0;
  int k = 0;
  while (i < stride && j < stride) {
    if (src1[i] < src2[j]) {
      dst[k] = src1[i];
      dst[k + 1] = src1[i + 1];
      i += 2;
    } else {
      dst[k] = src2[j];
      dst[k + 1] = src2[j + 1];
      j += 2;
    }
    k += 2;
  }
  while (i < stride) {
    dst[k] = src1[i];
    dst[k + 1] = src1[i + 1];
    i += 2;
    k += 2;
  }
  while (j < stride) {
    dst[k] = src2[j];
    dst[k + 1] = src2[j + 1];
    j += 2;
    k += 2;
  }
}

void matrix_prepare(int32_t* data, int32_t data_init, int32_t data_size,
                    int32_t** txmem_p, int32_t* size_p)
{
  for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i++)
    msortMem[i] = data[i / (2 * BLOWUPFACTOR) * 2 * BLOWUPFACTOR * SqrtN +
                       i % (2 * BLOWUPFACTOR) + data_init * 2 * BLOWUPFACTOR];
  *txmem_p = msortMem;
  *size_p = 2 * SqrtN * BLOWUPFACTOR;
}


int checkOFlow(int32_t* aPerm) {
  int count[SqrtN];
  int bucket;
  int max = 0;
  for (int i = 0; i < SqrtN; i++) {
    memset(count, 0, sizeof(int32_t) * SqrtN);
    for (int j = 0; j < SqrtN; j++) {
      bucket = aPerm[i * SqrtN + j] / SqrtN;
      if (count[bucket] == BLOWUPFACTOR) return 1;
      count[bucket]++;
    }
  }
  return 0;
}
void genRandom(int* permutation) {
  unsigned int seed;
  for (int i = 0; i < N; i++) permutation[i] = i;
  for (int i = 0; i <= N - 2; i++) {
    sgx_read_rand((unsigned char*)&seed, 4);
    int j = seed % (N - i);
    swap(&permutation[i], &permutation[i + j]);
  }

}
int verify(int32_t* data, int32_t* perm, int32_t* output) {
  for (int i = 0; i < N; i++)
    if (output[perm[i]] != data[i]) return 1;
  return 0;
}

int compare(const void* a, const void* b) { return (*(int*)a - *(int*)b); }
int32_t* apptxs_cleanup_bsort(int32_t* data, int32_t data_init,
    int32_t data_size);

int32_t* apptxs_cleanup_msort(int32_t* data, int32_t data_init,
                              int32_t data_size)
{
  int32_t* txmem2 = 0;
  int32_t txmem_size;
  matrix_prepare(data, data_init, data_size, &txmem2, &txmem_size);
  int32_t* input;
  int32_t* output;
  input = txmem2;
  output = tmp_data;
  int32_t* swap;
  for (int stride = 2; stride < data_size; stride *= 2) {
    for (int j = 0; j < data_size; j += 2 * stride) {
    //  apptx_merge(output + j, input + j, input + j + stride, stride);
     c_merge(output+j,input+j,input+j+stride,stride);
    }
    swap = input;
    input = output;
    output = swap;
  }
  return input;
}

void app_distribute_coda(HANDLE h1, HANDLE h2,HANDLE h3) {
  int32_t perm = 0;
  int32_t data = 0;
  int32_t off = 0;
  int32_t cur = 0;
  for (int i = 0; i < SqrtN; i++) {
    data = ob_read_next(h1);
    perm = ob_read_next(h2);
    off = (perm / SqrtN) * (h3 * BLOWUPFACTOR + 1);
    cur = nob_read_at(h3,off);
    nob_write_at(h3,off+1+cur,perm);
    nob_write_at(h3,off+1+cur+1,data);
    cur += 2;
    nob_write_at(h3,off,cur);
  }
}

int coda_distribute(int32_t* M_data, int32_t M_data_init, int32_t M_data_size,
    int32_t* M_perm, int32_t M_perm_init, int32_t* M_output,
    int32_t M_output_init, int32_t M_output_size)
{
  HANDLE dataIter = initialize_ob_iterator(&M_data[SqrtN*M_data_init],SqrtN);
  HANDLE permIter = initialize_ob_iterator(&M_perm[SqrtN*M_data_init],SqrtN);
  DATA output[(2*BLOWUPFACTOR)*SqrtN];
  for(int i=0;i<SqrtN;i++) {
    for (int j=0;j<2*BLOWUPFACTOR;j++)
      output[(2*BLOWUPFACTOR)*i+j] = -1;
  }
  int d,p;
  melshuffle_element_t e;
  MultiQueue<melshuffle_element_t> queues(QUEUE_CAP,SqrtN);  
  queues.init_nob();
  coda_txbegin();
  for(int i=0;i<SqrtN;i++) {
    d = ob_read_next(dataIter);
    p = ob_read_next(permIter);
    //EPrintf("i=%d,p=%d\n",i,p);
    melshuffle_element_t e;
    e.value = d;
    e.perm = p;
    queues.push_back(p/SqrtN,e);
  }
  coda_txend();
  queues.reset_nob();
  for(int i=0;i<SqrtN;i++) {
    queues.init_nob();
    HANDLE ob_out = initialize_ob_rw_iterator(output+i*(2*BLOWUPFACTOR),2*BLOWUPFACTOR);
    coda_txbegin();
    int count = 0;
    while (!queues.empty(i)) {
      queues.front(i,&e);
      //EPrintf("pop out i=%d, e.value=%d,e.perm=%d\n",i,e.value,e.perm);
      ob_rw_write_next(ob_out,e.perm);
      ob_rw_write_next(ob_out,e.value);
      queues.pop_front(i);
      count++;
    }
    coda_txend();
    queues.reset_nob();
    int q=0;
    for(int p=0;p<2*count;p++) {
      (output+(i*2*BLOWUPFACTOR))[p] = ob_rw_read_next(ob_out);
    }
    for(int p=0;p<2*count;p++) {
     // EPrintf("%d\n",(output+(i*2*BLOWUPFACTOR))[p]);
    }
  }
  for (int i = 0; i < SqrtN; i++)
    for (int j = 0; j < 2 * BLOWUPFACTOR; j++) {
      int ret = output[i*(2*BLOWUPFACTOR)+j];
      M_output[M_output_init * SqrtN * 2 * BLOWUPFACTOR + i * 2 * BLOWUPFACTOR +
        j] = ret;//nob_read_at(outputArray,i * (2 * BLOWUPFACTOR + 1) + j + 1);
    }
}

int coda_melshuffle(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size,
    int input_size)
{
  // input validation and method selection
//  if (!(input_size == 2 * 2 || input_size == 4 * 4 || input_size == 8 * 8 ||
//        input_size == 16 * 16 || input_size == 32 * 32 ||
//        input_size == 64 * 64 || input_size == 128 * 128))
//    return 0;

  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;
  int M_random[N];
  int M_rr[N];
  int M_dr[N];
  long sec_begin[1], sec_end[1], usec_begin[1], usec_end[1];
  ocall_gettimenow(sec_begin, usec_begin);

  while (1) {
    genRandom(M_random);
    /* shuffle pass 1  PiR = shuffle(Pi,R);*/

    if (checkOFlow(M_random)) {
      EPrintf("random overflow\n");
      continue;
    }
    for (int i = 0; i < SqrtN; i++) {
      coda_distribute(M_perm, i, SqrtN, M_random, i, g_interm, i,
          SqrtN * BLOWUPFACTOR);
    }

    //for(int i=0;i<N*2*BLOWUPFACTOR;i++)
    // EPrintf("g_interm[%d]=%d\n",i,g_interm[i]);
    /* unit test */
    // testMergeSort();
    int pos = 0;
    for (int j = 0; j < SqrtN; j++) {
      int32_t* ret =
        apptxs_cleanup_msort(g_interm, j, 2 * SqrtN * BLOWUPFACTOR);
      for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR - 1; i += 2)
        if (ret[i] != -1) {
          M_rr[pos] = ret[i + 1];
          pos++;
        }
    }
    int aPoint = verify(M_perm, M_random, M_rr);
    if (!aPoint)
      EPrintf("distrbute correct!\n");
    else
      EPrintf("distribute Wrong\n");

    if (checkOFlow(M_rr)) {
      EPrintf("pi_r overflow\n");
      continue;
    }

    // shuffle pass 2  Dr = shuffle(D,R);
    for (int i = 0; i < SqrtN; i++) {
      coda_distribute(M_data, i, SqrtN, M_random, i, g_interm, i,
          SqrtN * BLOWUPFACTOR);
    }
    pos = 0;
    for (int j = 0; j < SqrtN; j++) {
      int32_t* ret =
        apptxs_cleanup_msort(g_interm, j, 2 * SqrtN * BLOWUPFACTOR);
      for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i += 2)
        if (ret[i] != -1) {
          M_dr[pos] = ret[i + 1];
          pos++;
        }
    }

    // shuffle pass 3   O = shuffle(Dr,PiR)
    for (int i = 0; i < SqrtN; i++) {
      coda_distribute(M_dr, i, SqrtN, M_rr, i, g_interm, i,
          SqrtN * BLOWUPFACTOR);
    }
    pos = 0;
    for (int j = 0; j < SqrtN; j++) {
      int32_t* ret =
        apptxs_cleanup_msort(g_interm, j, 2 * SqrtN * BLOWUPFACTOR);
      for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i += 2)
        if (ret[i] != -1) {
          M_output[pos] = ret[i + 1];
          pos++;
        }
    }
    break;
  }
  //qsort(M_output, N, sizeof(int), compare);
  ocall_gettimenow(sec_end, usec_end);
  EPrintf("shuffle time = %ld\n",(sec_end[0]*1000000+usec_end[0]) - (sec_begin[0]*1000000+usec_begin[0]));
  return 0;
}
