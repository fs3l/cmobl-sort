#include <math.h>
#include <sgx_trts.h>
#include <stdio.h>
#include <stdlib.h>

#include "./Enclave.h"
#include "./RealLibCoda.h"
#include "./cache_shuffle.h"

//#define DEBUG_PRINTF(...)
#define DEBUG_PRINTF(...) EPrintf(__VA_ARGS__)

#define EPSILON 0.5
#define SPRAY_MAP_MAX_LEN 30  // > (1 + 1/EPSILON) ln(2e)

static __attribute__((always_inline)) inline int32_t min(int32_t a, int32_t b)
{
  return a < b ? a : b;
}

static __attribute__((always_inline)) inline int32_t max(int32_t a, int32_t b)
{
  return a > b ? a : b;
}

class CacheShuffleMap
{
public:
  CacheShuffleMap(int32_t num_of_map, int32_t max_len)
  {
    int32_t data_len = 2 + num_of_map * (max_len * 2 + 1);
    data = new int32_t[data_len];
    memset(data, 0, sizeof(int32_t) * data_len);
    data[0] = num_of_map;
    data[1] = max_len;
  }
  ~CacheShuffleMap() { delete[] data; }
  __attribute__((always_inline)) inline void init_nob()
  {
    nob = initialize_nob_array(data, 2 + data[0] * (data[1] * 2 + 1));
  }
  __attribute__((always_inline)) inline void reset_nob()
  {
    for (int32_t i = 2; i < 2 + data[0] * (data[1] * 2 + 1); ++i)
      data[i] = nob_read_at(nob, i);
  }

  __attribute__((always_inline)) inline int32_t num_of_map() const
  {
    // return data[0];
    return nob_read_at(nob, 0);
  }
  __attribute__((always_inline)) inline int32_t max_len() const
  {
    // return data[1];
    return nob_read_at(nob, 1);
  }
  __attribute__((always_inline)) inline int32_t size_idx(int32_t id) const
  {
    if (id >= num_of_map()) abort();
    return 2 + id * (max_len() * 2 + 1);
  }
  __attribute__((always_inline)) inline int32_t value_idx(
      int32_t id, int32_t value_id) const
  {
    if (id >= num_of_map()) abort();
    if (value_id >= max_len()) abort();
    return 3 + id * (max_len() * 2 + 1) + value_id * 2;
  }
  __attribute__((always_inline)) inline int32_t perm_idx(int32_t id,
                                                         int32_t value_id) const
  {
    if (id >= num_of_map()) abort();
    if (value_id >= max_len()) abort();
    return 4 + id * (max_len() * 2 + 1) + value_id * 2;
  }

  __attribute__((always_inline)) inline int32_t size(int32_t id) const
  {
    // return data[size_idx(id)];
    return nob_read_at(nob, size_idx(id));
  }
  __attribute__((always_inline)) inline bool empty(int32_t id) const
  {
    return size(id) == 0;
  }

  __attribute__((always_inline)) inline void add(int32_t id, int32_t value,
                                                 int32_t perm)
  {
    int32_t cur_size = size(id);
    // data[value_idx(id, cur_size)] = value;
    // data[perm_idx(id, cur_size)] = perm;
    // data[size_idx(id)] = cur_size + 1;
    nob_write_at(nob, value_idx(id, cur_size), value);
    nob_write_at(nob, perm_idx(id, cur_size), perm);
    nob_write_at(nob, size_idx(id), cur_size + 1);
  }
  __attribute__((always_inline)) inline void top(int32_t id, int32_t* value,
                                                 int32_t* perm) const
  {
    int32_t cur_size = size(id);
    if (cur_size <= 0) abort();
    //*value = data[value_idx(id, cur_size - 1)];
    //*perm = data[perm_idx(id, cur_size - 1)];
    *value = nob_read_at(nob, value_idx(id, cur_size - 1));
    *perm = nob_read_at(nob, perm_idx(id, cur_size - 1));
  }
  __attribute__((always_inline)) inline void pop(int32_t id)
  {
    int32_t cur_size = size(id);
    if (cur_size <= 0) abort();
    // data[size_idx(id)] = cur_size - 1;
    nob_write_at(nob, size_idx(id), cur_size - 1);
  }

private:
  // data layout: [num_of_map] [max_len] [m_1] .... [m_n]
  //    m layout: [cur_size] [value_1] [perm_1] ... [value_m] [perm_m]
  int32_t* data;
  HANDLE nob;
};

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
  __attribute__((always_inline)) inline void init_read_ob(int32_t ob_offset,
                                                          int32_t ob_len,
                                                          HANDLE* arr_ob,
                                                          HANDLE* perm_ob)
  {
    *arr_ob = initialize_ob_iterator(arr + ob_offset, ob_len);
    *perm_ob = initialize_ob_iterator(perm + ob_offset, ob_len);
  }
  __attribute__((always_inline)) inline void init_rw_ob(HANDLE* arr_ob,
                                                        HANDLE* perm_ob)
  {
    *arr_ob = initialize_ob_rw_iterator(arr, len);
    *perm_ob = initialize_ob_rw_iterator(perm, len);
  }
  __attribute__((always_inline)) inline void reset_rw_ob(HANDLE* arr_ob,
                                                         HANDLE* perm_ob)
  {
    for (int32_t i = 0; i < len; ++i) {
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

  __attribute__((always_inline)) inline void random_shuffle()
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

  __attribute__((always_inline)) inline CacheShuffleData** spary(
      const int32_t in_partitions, const int32_t out_partitions)
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

    int32_t i, j, in_idx, v, p;

    for (i = 0; i < out_partitions; ++i) {
      int32_t result_i_begin_idx = out_p_idx_len * i + begin_idx;
      int32_t result_i_end_idx =
          min(out_p_idx_len * (i + 1) + begin_idx, end_idx);
      result[i] =
          new CacheShuffleData(out_p_len, result_i_begin_idx, result_i_end_idx);
      DEBUG_PRINTF("out[%d] idx=[%d, %d)\n", i, result[i]->begin_idx,
                   result[i]->end_idx);
      if (result[i]->begin_idx >= result[i]->end_idx) abort();
    }

    CacheShuffleMap nob_map(out_partitions, min(SPRAY_MAP_MAX_LEN, out_p_len));

    for (i = 0; i < in_partitions; ++i) {
      int32_t* out_arr = new int32_t[out_partitions];
      int32_t* out_perm = new int32_t[out_partitions];
      nob_map.init_nob();
      HANDLE in_arr_ob, in_perm_ob;
      init_read_ob(i * in_p_len, min(in_p_len, len - (i - 1) * in_p_len),
                   &in_arr_ob, &in_perm_ob);
      HANDLE out_arr_ob = initialize_ob_rw_iterator(out_arr, out_partitions);
      HANDLE out_perm_ob = initialize_ob_rw_iterator(out_perm, out_partitions);
      coda_txbegin();
      for (j = 0; j < in_p_len; ++j) {
        in_idx = i * in_p_len + j;
        if (in_idx < len) {
          // v = arr[in_idx];
          // p = perm[in_idx];
          v = ob_read_next(in_arr_ob);
          p = ob_read_next(in_perm_ob);
          if (p != -1) nob_map.add((p - begin_idx) / out_p_idx_len, v, p);
        }
      }

      for (j = 0; j < out_partitions; ++j) {
        v = p = -1;
        if (!nob_map.empty(j)) {
          nob_map.top(j, &v, &p);
          nob_map.pop(j);
        }
        // result[j]->arr[i] = v;
        // result[j]->perm[i] = p;
        ob_rw_write_next(out_arr_ob, v);
        ob_rw_write_next(out_perm_ob, p);
      }
      coda_txend();
      nob_map.reset_nob();

      for (j = 0; j < out_partitions; ++j) {
        result[j]->arr[i] = ob_rw_read_next(out_arr_ob);
        result[j]->perm[i] = ob_rw_read_next(out_perm_ob);
      }

      delete[] out_arr;
      delete[] out_perm;
    }

    for (i = 0; i < out_partitions; ++i) {
      nob_map.init_nob();
      HANDLE in_arr_ob, in_perm_ob, out_arr_ob, out_perm_ob;
      result[i]->init_read_ob(0, out_p_len, &in_arr_ob, &in_perm_ob);
      result[i]->init_rw_ob(&out_arr_ob, &out_perm_ob);
      coda_txbegin();
      for (j = 0; j < out_p_len; ++j) {
        // v = result[i]->arr[j];
        // p = result[i]->perm[j];
        v = ob_read_next(in_arr_ob);
        p = ob_read_next(in_perm_ob);
        if (p == -1 && !nob_map.empty(i)) {
          nob_map.top(i, &v, &p);
          nob_map.pop(i);
        }
        // result[i]->arr[j] = v;
        // result[i]->perm[j] = p;
        ob_rw_write_next(out_arr_ob, v);
        ob_rw_write_next(out_perm_ob, p);
      }

      if (!nob_map.empty(i)) abort();
      coda_txend();
      nob_map.reset_nob();
      result[i]->reset_rw_ob(&out_arr_ob, &out_perm_ob);
    }

    return result;
  }

  __attribute__((always_inline)) inline int32_t compute_spary_in_partitions(
      int32_t s)
  {
    return min(s, len);
  }
  __attribute__((always_inline)) inline int32_t compute_spary_out_partitions(
      int32_t Q)
  {
    Q = min(Q, len);
    Q = min(Q, ceil((double)len / ceil((double)len / Q)));
    return Q;
  }
  __attribute__((always_inline)) inline int32_t compute_rspary_partitions(
      int32_t S)
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
      data->spary(data->compute_spary_in_partitions(len / S), temp_len);
  delete data;

  // rspary
  int32_t new_temp_len;

  while (temp_len < len) {
    new_temp_len = 0;
    for (int32_t i = 0; i < temp_len; ++i) {
      new_temp_len += temp[i]->compute_rspary_partitions(S);
    }

    CacheShuffleData** new_temp = new CacheShuffleData*[new_temp_len];

    for (int32_t i = 0, j = 0; i < temp_len; ++i) {
      int32_t rspary_partitions = temp[i]->compute_rspary_partitions(S);
      if (rspary_partitions == 1) {
        new_temp[j++] = temp[i];
      } else {
        temp[i]->random_shuffle();
        CacheShuffleData** temp2 =
            temp[i]->spary(rspary_partitions, rspary_partitions);
        delete temp[i];
        for (int32_t k = 0; k < rspary_partitions; ++k)
          new_temp[j++] = temp2[k];
        delete[] temp2;
      }
    }
    delete[] temp;
    temp = new_temp;
    temp_len = new_temp_len;
  }

  DEBUG_PRINTF("temp_len=%d\n", temp_len);
  if (temp_len != len) abort();

  for (int32_t i = 0; i < temp_len; ++i) {
    int32_t j, v, p, real_v;
    HANDLE arr_ob, perm_ob;
    temp[i]->init_read_ob(0, temp[i]->len, &arr_ob, &perm_ob);
    coda_txbegin();
    for (j = 0; j < temp[i]->len; ++j) {
      // v = temp[i]->arr[j];
      // p = temp[i]->perm[j];
      v = ob_read_next(arr_ob);
      p = ob_read_next(perm_ob);
      if (p != -1) {
        real_v = v;
      }
    }
    coda_txend();
    arr_out[i] = real_v;
    delete temp[i];
  }

  delete[] temp;
}

static void print_arr(const int32_t* arr, int32_t len)
{
  for (int32_t i = 0; i < len; ++i) EPrintf("%d ", arr[i]);
  EPrintf("\n");
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
  int32_t len = 10;
  int32_t* data = gen_arr(len);
  print_arr(data, len);
  int32_t* data_out = new int32_t[len];

  cache_shuffle(data, data, data_out, len);
  print_arr(data_out, len);

  delete[] data;
  delete[] data_out;
}
