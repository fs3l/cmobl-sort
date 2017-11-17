#include <math.h>
#include <sgx_trts.h>
#include <stdio.h>
#include <stdlib.h>

#include "./Enclave.h"
#include "./RealLibCoda.h"
#include "./cache_shuffle.h"
#include "./multiqueue.hpp"
#include "./utility.h"

//#define DEBUG_PRINTF(...)
#define DEBUG_PRINTF(...) EPrintf(__VA_ARGS__)

#define EPSILON 2
#define QUEUE_CAP 1000

struct cache_shuffle_element {
  int32_t value;
  int32_t perm;
};
typedef struct cache_shuffle_element cache_shuffle_element_t;

class CacheShuffleData
{
public:
  CacheShuffleData(int32_t len, int32_t begin_idx, int32_t end_idx)
      : len(len), begin_idx(begin_idx), end_idx(end_idx)
  {
    arr = new int32_t[len];
    perm = new int32_t[len];
    for (int32_t i = 0; i < len; ++i) {
      perm[i] = -1;
    }
  }
  CacheShuffleData(int32_t len, int32_t begin_idx, int32_t end_idx,
                   const int32_t* arr_in, const int32_t* perm_in)
      : len(len), begin_idx(begin_idx), end_idx(end_idx)
  {
    arr = new int32_t[len];
    perm = new int32_t[len];
    for (int32_t i = 0; i < len; ++i) {
      arr[i] = arr_in[i];
      perm[i] = perm_in[i];
    }
  }
  ~CacheShuffleData()
  {
    delete[] arr;
    delete[] perm;
  }
  void init_read_ob(int32_t ob_offset, int32_t ob_len, HANDLE* arr_ob,
                    HANDLE* perm_ob)
  {
    *arr_ob = initialize_ob_iterator(arr + ob_offset, ob_len);
    *perm_ob = initialize_ob_iterator(perm + ob_offset, ob_len);
  }
  void init_rw_ob(int32_t ob_offset, int32_t ob_len, HANDLE* arr_ob,
                  HANDLE* perm_ob)
  {
    *arr_ob = initialize_ob_rw_iterator(arr + ob_offset, ob_len);
    *perm_ob = initialize_ob_rw_iterator(perm + ob_offset, ob_len);
  }
  void reset_rw_ob(int32_t ob_offset, int32_t ob_len, HANDLE* arr_ob,
                   HANDLE* perm_ob)
  {
    for (int32_t i = ob_offset; i < ob_offset + ob_len; ++i) {
      arr[i] = ob_rw_read_next(*arr_ob);
      perm[i] = ob_rw_read_next(*perm_ob);
    }
  }

  void print() const
  {
    DEBUG_PRINTF("Data[len=%d,begin_idx=%d,end_idx=%d]:\n", len, begin_idx,
                 end_idx);
    for (int32_t i = 0; i < len; ++i)
      DEBUG_PRINTF("p=%d,v=%d ", perm[i], arr[i]);
    DEBUG_PRINTF("\n");
  }

  void random_shuffle()
  {
    int32_t i, j, temp;
    for (i = 0; i < len - 1; ++i) {
      sgx_read_rand((unsigned char*)&j, sizeof(int32_t));
      j = abs(j) % len;
      // swap(i, j)
      temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
      temp = perm[i];
      perm[i] = perm[j];
      perm[j] = temp;
    }
  }

  CacheShuffleData** spary(const int32_t in_partitions,
                           const int32_t out_partitions)
  {
    const int32_t in_p_len = (int32_t)ceil((double)len / (double)in_partitions);
    const int32_t out_p_len =
        max((int32_t)ceil((double)len / (double)out_partitions), in_partitions);
    const int32_t out_p_idx_len =
        (int32_t)ceil((double)(end_idx - begin_idx) / (double)out_partitions);

    CacheShuffleData** result = new CacheShuffleData*[out_partitions];

    DEBUG_PRINTF("-----------------------------------\n");
    DEBUG_PRINTF("spary #in=%d, #out=%d, idx=[%d, %d)\n", in_partitions,
                 out_partitions, begin_idx, end_idx);
    DEBUG_PRINTF("in_p_len=%d, out_p_len=%d, out_p_idx_len=%d\n", in_p_len,
                 out_p_len, out_p_idx_len);

    int32_t i, j, v, p, read_ob_len;
    cache_shuffle_element_t e;

    for (i = 0; i < out_partitions; ++i) {
      int32_t result_i_begin_idx = out_p_idx_len * i + begin_idx;
      int32_t result_i_end_idx =
          min(out_p_idx_len * (i + 1) + begin_idx, end_idx);
      result[i] =
          new CacheShuffleData(out_p_len, result_i_begin_idx, result_i_end_idx);
      // DEBUG_PRINTF("out[%d] idx=[%d, %d)\n", i, result[i]->begin_idx,
      // result[i]->end_idx);
      if (result[i]->begin_idx >= result[i]->end_idx)
        Eabort("invalid partition");
    }

    MultiQueue<cache_shuffle_element_t> queues(QUEUE_CAP, out_partitions);

    for (i = 0; i < in_partitions; ++i) {
      read_ob_len = min(in_p_len, max(len - i * in_p_len, 0));
      if (read_ob_len <= 0) break;
      queues.init_nob();
      HANDLE in_arr_ob, in_perm_ob;
      init_read_ob(i * in_p_len, read_ob_len, &in_arr_ob, &in_perm_ob);
      coda_txbegin();
      for (j = 0; j < read_ob_len; ++j) {
        v = ob_read_next(in_arr_ob);
        p = ob_read_next(in_perm_ob);
        if (p != -1) {
          if (queues.full()) {
            coda_txend();
            Eabort("queues full.");
          }
          e.value = v;
          e.perm = p;
          queues.push_back((p - begin_idx) / out_p_idx_len, e);
        }
      }
      coda_txend();
      queues.reset_nob();

      int32_t write_len = 0;
      while (write_len < out_partitions) {
        int32_t step = min(out_partitions - write_len, max_ob_write());
        queues.init_nob();
        int32_t* out_arr = new int32_t[step];
        int32_t* out_perm = new int32_t[step];
        HANDLE out_arr_ob = initialize_ob_rw_iterator(out_arr, step);
        HANDLE out_perm_ob = initialize_ob_rw_iterator(out_perm, step);
        coda_txbegin();
        for (j = write_len; j < step + write_len; ++j) {
          v = p = -1;
          if (!queues.empty(j)) {
            queues.front(j, &e);
            v = e.value;
            p = e.perm;
            queues.pop_front(j);
          }
          ob_rw_write_next(out_arr_ob, v);
          ob_rw_write_next(out_perm_ob, p);
        }
        coda_txend();
        queues.reset_nob();

        for (j = write_len; j < step + write_len; ++j) {
          result[j]->arr[i] = ob_rw_read_next(out_arr_ob);
          result[j]->perm[i] = ob_rw_read_next(out_perm_ob);
        }

        delete[] out_arr;
        delete[] out_perm;
        write_len += step;
      }
    }

    for (i = 0; i < out_partitions; ++i) {
      int32_t write_len = 0;
      while (write_len < out_p_len) {
        int32_t step = min(out_p_len - write_len, max_ob_write());
        queues.init_nob();
        HANDLE in_arr_ob, in_perm_ob, out_arr_ob, out_perm_ob;
        result[i]->init_read_ob(write_len, step, &in_arr_ob, &in_perm_ob);
        result[i]->init_rw_ob(write_len, step, &out_arr_ob, &out_perm_ob);
        coda_txbegin();
        for (j = write_len; j < step + write_len; ++j) {
          v = ob_read_next(in_arr_ob);
          p = ob_read_next(in_perm_ob);
          if (p == -1 && !queues.empty(i)) {
            queues.front(i, &e);
            v = e.value;
            p = e.perm;
            queues.pop_front(i);
          }
          ob_rw_write_next(out_arr_ob, v);
          ob_rw_write_next(out_perm_ob, p);
        }
        coda_txend();
        queues.reset_nob();
        result[i]->reset_rw_ob(write_len, step, &out_arr_ob, &out_perm_ob);
        write_len += step;
      }

      bool nob_empty;
      queues.init_nob();
      coda_txbegin();
      nob_empty = queues.empty(i);
      coda_txend();
      if (!nob_empty) Eabort("queue %d not empty", i);
    }

    return result;
  }

  int32_t compute_spary_in_partitions(int32_t S)
  {
    return min(ceil((double)len / S), len);
  }
  int32_t compute_spary_out_partitions(int32_t S)
  {
    int32_t idx_len = end_idx - begin_idx;
    S = min(S, max(idx_len, 1));
    S = min(S, ceil((double)idx_len / ceil((double)idx_len / S)));
    return S;
  }

  int32_t len, begin_idx, end_idx;
  int32_t* arr;
  int32_t* perm;
};

void cache_shuffle(const int32_t* arr_in, const int32_t* perm_in,
                   int32_t* arr_out, int32_t len)
{
  if (len == 1) {
    arr_out[0] = arr_in[0];
    return;
  }

  const int32_t S = max(1, (int32_t)log2((double)len));
  const int32_t Q = (int32_t)ceil((1 + EPSILON) * (double)S);

  // spary
  CacheShuffleData* data = new CacheShuffleData(len, 0, len, arr_in, perm_in);
  int32_t temp_len = data->compute_spary_out_partitions(Q);
  CacheShuffleData** temp =
      data->spary(data->compute_spary_in_partitions(S), temp_len);
  delete data;

  // rspary
  int32_t new_temp_len;

  while (temp_len < len) {
    new_temp_len = 0;
    for (int32_t i = 0; i < temp_len; ++i) {
      new_temp_len += temp[i]->compute_spary_out_partitions(S);
    }

    CacheShuffleData** new_temp = new CacheShuffleData*[new_temp_len];

    for (int32_t i = 0, j = 0; i < temp_len; ++i) {
      int32_t rspary_out_partitions = temp[i]->compute_spary_out_partitions(S);
      int32_t rspary_in_partitions =
          temp[i]->compute_spary_in_partitions(rspary_out_partitions);
      if (rspary_out_partitions == 1) {
        new_temp[j++] = temp[i];
      } else {
        temp[i]->random_shuffle();
        CacheShuffleData** temp2 =
            temp[i]->spary(rspary_in_partitions, rspary_out_partitions);
        delete temp[i];
        for (int32_t k = 0; k < rspary_out_partitions; ++k)
          new_temp[j++] = temp2[k];
        delete[] temp2;
      }
    }
    delete[] temp;
    temp = new_temp;
    temp_len = new_temp_len;
  }

  DEBUG_PRINTF("temp_len=%d\n", temp_len);
  if (temp_len != len) Eabort("invalid partition size");

  for (int32_t i = 0; i < temp_len; ++i) {
    for (int32_t j = 0; j < temp[i]->len; ++j) {
      int32_t v = temp[i]->arr[j];
      int32_t p = temp[i]->perm[j];
      cmove_int32(p != -1, &v, &arr_out[i]);
    }
    delete temp[i];
  }

  delete[] temp;
}

static int32_t* gen_arr(int32_t len)
{
  int32_t* arr = new int32_t[len];
  for (int32_t i = 0; i < len; ++i) arr[i] = i;
  for (int32_t i = 0; i < len - 1; ++i) {
    int32_t j;
    sgx_read_rand((unsigned char*)&j, sizeof(int32_t));
    j = abs(j) % len;
    int32_t temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
  return arr;
}

void cache_shuffle_test()
{
  int32_t len = 1000000;
  int32_t* data = gen_arr(len);
  // print_arr(data, len);
  int32_t* data_out = new int32_t[len];

  cache_shuffle(data, data, data_out, len);
  // print_arr(data_out, len);
  EPrintf("done\n");

  delete[] data;
  delete[] data_out;
}
