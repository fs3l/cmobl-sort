/*
 * Author: Ju Chen
 * 10/24/2017
*/

#include "./Enclave.h"
#include "./Enclave_t.h"
#include "./RealLibCoda.h"


int32_t g_interm[2 * BLOWUPFACTOR * N];

int checkOFlow(int32_t* aPerm);
void genRandom(int* permutation);
int verify(int32_t* data, int32_t* perm, int32_t* output);

int compare(const void* a, const void* b); 
int32_t* apptxs_cleanup_bsort(int32_t* data, int32_t data_init,
    int32_t data_size);
int32_t* apptxs_cleanup_msort(int32_t* data, int32_t data_init,
    int32_t data_size);
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
  DATA output[(2*BLOWUPFACTOR+1)*SqrtN];
  for(int i=0;i<SqrtN;i++) {
    output[(2*BLOWUPFACTOR+1)*i] =0;
    for (int j=1;j<2*BLOWUPFACTOR+1;j++)
      output[(2*BLOWUPFACTOR+1)*i+j] = -1;
  }
  HANDLE outputArray = initialize_nob_array(output,(2*BLOWUPFACTOR+1)*SqrtN);

  coda_txbegin();
  app_distribute_coda(dataIter,permIter,outputArray);
  coda_txend();
  for (int i = 0; i < SqrtN; i++)
    for (int j = 0; j < 2 * BLOWUPFACTOR; j++) {
      int ret = nob_read_at(outputArray,i*(2*BLOWUPFACTOR+1)+j+1);
      M_output[M_output_init * SqrtN * 2 * BLOWUPFACTOR + i * 2 * BLOWUPFACTOR +
        j] = ret;//nob_read_at(outputArray,i * (2 * BLOWUPFACTOR + 1) + j + 1);
    }
}

int coda_melshuffle(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size,
    int input_size)
{
  // input validation and method selection
  if (!(input_size == 2 * 2 || input_size == 4 * 4 || input_size == 8 * 8 ||
        input_size == 16 * 16 || input_size == 32 * 32 ||
        input_size == 64 * 64 || input_size == 128 * 128))
    return 0;

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
  qsort(M_output, N, sizeof(int), compare);
  ocall_gettimenow(sec_end, usec_end);
  EPrintf("shuffle time = %ld\n",(sec_end[0]*1000000+usec_end[0]) - (sec_begin[0]*1000000+usec_begin[0]));
  return 0;
}
