/*
  Author: Cheng Xu
  Integration: Ju Chen
  10/04/2017
*/

#include <math.h>
#include <sgx_trts.h>
#include <stdio.h>
#include <stdlib.h>

#include "./Enclave.h"
#include "./RealLibCoda.h"
#include "./sort.h"
#define C 4

class Queue
{
public:
  Queue(const int32_t max_len)
  {
    data = new int32_t[max_len + 3];
    data[0] = max_len;
    data[1] = 0;
    data[2] = 0;
  }
  ~Queue() { delete[] data; }
  __attribute__((always_inline)) inline void init_nob()
  {
    nob = initialize_nob_array(data, data[0] + 3);
  }
  __attribute__((always_inline)) inline void reset_nob()
  {
    for (int32_t i = 1; i < data[0] + 3; ++i) data[i] = nob_read_at(nob, i);
  }

  __attribute__((always_inline)) inline int32_t get_max_len() const
  {
    return nob_read_at(nob, 0);
  }
  __attribute__((always_inline)) inline int32_t get_cur_len() const
  {
    return nob_read_at(nob, 1);
  }
  __attribute__((always_inline)) inline int32_t get_front_idx() const
  {
    return nob_read_at(nob, 2);
  }
  __attribute__((always_inline)) inline void set_cur_len(int32_t cur_len)
  {
    nob_write_at(nob, 1, cur_len);
  }
  __attribute__((always_inline)) inline void set_front_idx(int32_t front_idx)
  {
    nob_write_at(nob, 2, front_idx);
  }

  __attribute__((always_inline)) inline void enqueue(const int32_t val)
  {
    int32_t max_len = get_max_len();
    int32_t cur_len = get_cur_len();
    int32_t front_idx = get_front_idx();
    if (cur_len == max_len) abort();
    nob_write_at(nob, (front_idx + cur_len) % max_len + 3, val);
    ++cur_len;
    set_cur_len(cur_len);
  }
  __attribute__((always_inline)) inline int32_t front() const
  {
    int32_t cur_len = get_cur_len();
    int32_t front_idx = get_front_idx();
    if (cur_len == 0) abort();
    return nob_read_at(nob, front_idx + 3);
  }
  __attribute__((always_inline)) inline void dequeue()
  {
    int32_t max_len = get_max_len();
    int32_t cur_len = get_cur_len();
    int32_t front_idx = get_front_idx();
    if (cur_len == 0) abort();
    --cur_len;
    front_idx = (front_idx + 1) % max_len;
    set_cur_len(cur_len);
    set_front_idx(front_idx);
  }

private:
  // data_layout:  max_len, cur_len, front_idx, data...
  int32_t* data;
  HANDLE nob;
};

static __attribute__((always_inline)) inline void reset_tx(
    Queue* lhs_q, Queue* rhs_q, int32_t* lhs_out, int32_t* rhs_out,
    const int32_t lhs_len, const int32_t rhs_len, const int32_t lhs_out_idx,
    const int32_t rhs_out_idx, int32_t* arr_out, const int32_t len,
    const int32_t arr_out_idx, HANDLE* lhs_out_ob, HANDLE* rhs_out_ob,
    HANDLE* arr_out_ob, int32_t* ob_write, int32_t* max_ob_write)
{
  coda_txend();
  lhs_q->reset_nob();
  rhs_q->reset_nob();
  for (int32_t i = 0; i < *ob_write; i++)
    arr_out[arr_out_idx - *ob_write + i] = ob_rw_read_next(*arr_out_ob);

  *lhs_out_ob =
      initialize_ob_iterator(lhs_out + lhs_out_idx, lhs_len - lhs_out_idx);
  *rhs_out_ob =
      initialize_ob_iterator(rhs_out + rhs_out_idx, rhs_len - rhs_out_idx);
  *arr_out_ob =
      initialize_ob_rw_iterator(arr_out + arr_out_idx, len - arr_out_idx);
  lhs_q->init_nob();
  rhs_q->init_nob();
  *ob_write = 0;
  *max_ob_write = 1;  // TODO: API to get max ob write number
  coda_txbegin();
}

void merge_sort(const int32_t* arr_in, int32_t* arr_out, const size_t len)
{
  if (len == 1) {
    arr_out[0] = arr_in[0];
    return;
  }

  size_t lhs_len = len / 2;
  size_t rhs_len = len - lhs_len;
  int32_t* lhs_out = new int32_t[lhs_len];
  int32_t* rhs_out = new int32_t[rhs_len];
  merge_sort(arr_in, lhs_out, lhs_len);
  merge_sort(arr_in + lhs_len, rhs_out, rhs_len);

  HANDLE lhs_out_ob = initialize_ob_iterator(lhs_out, lhs_len);
  HANDLE rhs_out_ob = initialize_ob_iterator(rhs_out, rhs_len);
  HANDLE arr_out_ob = initialize_ob_rw_iterator(arr_out, len);

  size_t s = C * (int)sqrt((double)len);
  size_t half_s = s / 2;
  Queue lhs_q(s), rhs_q(s);

  lhs_q.init_nob();
  rhs_q.init_nob();

  size_t i, j;
  int32_t lhs_val, rhs_val, out_val;

  int32_t ob_write = 0, lhs_out_idx = 0, rhs_out_idx = 0, arr_out_idx = 0;
  int32_t max_ob_write = 1;  // TODO: API to get max ob write number

  coda_txbegin();

  for (i = 0; i < half_s; ++i) {
    if (i < lhs_len) {
      lhs_q.enqueue(ob_read_next(lhs_out_ob));
      lhs_out_idx++;
    }
    if (i < rhs_len) {
      rhs_q.enqueue(ob_read_next(rhs_out_ob));
      rhs_out_idx++;
    }
  }
  for (; i < rhs_len + half_s; ++i) {
    if (i < lhs_len) {
      lhs_q.enqueue(ob_read_next(lhs_out_ob));
      lhs_out_idx++;
    }
    if (i < rhs_len) {
      rhs_q.enqueue(ob_read_next(rhs_out_ob));
      rhs_out_idx++;
    }

    for (j = 0; j < 2; ++j) {
      if (ob_write >= max_ob_write) {
        reset_tx(&lhs_q, &rhs_q, lhs_out, rhs_out, lhs_len, rhs_len,
                 lhs_out_idx, rhs_out_idx, arr_out, len, arr_out_idx,
                 &lhs_out_ob, &rhs_out_ob, &arr_out_ob, &ob_write,
                 &max_ob_write);
      }

      if (lhs_q.get_cur_len() > 0 && rhs_q.get_cur_len() > 0) {
        lhs_val = lhs_q.front();
        rhs_val = rhs_q.front();
        if (lhs_val < rhs_val) {
          out_val = lhs_val;
          lhs_q.dequeue();
        } else if (lhs_val == rhs_val) {
          out_val = lhs_val;
          if (lhs_q.get_cur_len() > rhs_q.get_cur_len())
            lhs_q.dequeue();
          else
            rhs_q.dequeue();
        } else {
          out_val = rhs_val;
          rhs_q.dequeue();
        }
        ob_rw_write_next(arr_out_ob, out_val);
        arr_out_idx++;
        ob_write++;
      } else {
        if (i < lhs_len || i < rhs_len) abort();
        goto done;
      }
    }
  }

  if (lhs_q.get_cur_len() > 0 && rhs_q.get_cur_len() > 0) abort();

done:

  while (lhs_q.get_cur_len() > 0) {
    if (ob_write >= max_ob_write) {
      reset_tx(&lhs_q, &rhs_q, lhs_out, rhs_out, lhs_len, rhs_len, lhs_out_idx,
               rhs_out_idx, arr_out, len, arr_out_idx, &lhs_out_ob, &rhs_out_ob,
               &arr_out_ob, &ob_write, &max_ob_write);
    }
    out_val = lhs_q.front();
    lhs_q.dequeue();
    ob_rw_write_next(arr_out_ob, out_val);
    arr_out_idx++;
    ob_write++;
  }

  while (rhs_q.get_cur_len() > 0) {
    if (ob_write >= max_ob_write) {
      reset_tx(&lhs_q, &rhs_q, lhs_out, rhs_out, lhs_len, rhs_len, lhs_out_idx,
               rhs_out_idx, arr_out, len, arr_out_idx, &lhs_out_ob, &rhs_out_ob,
               &arr_out_ob, &ob_write, &max_ob_write);
    }
    out_val = rhs_q.front();
    rhs_q.dequeue();
    ob_rw_write_next(arr_out_ob, out_val);
    arr_out_idx++;
    ob_write++;
  }

  coda_txend();

  for (int i = 0; i < ob_write; i++)
    arr_out[arr_out_idx - ob_write + i] = ob_rw_read_next(arr_out_ob);

  delete[] lhs_out;
  delete[] rhs_out;
}

int* random_arr(const int len)
{
  unsigned int rand_result;
  sgx_read_rand((unsigned char*)&rand_result, 4);
  int32_t* out = new int32_t[len];
  int i;
  for (i = 0; i < len; ++i) {
    sgx_read_rand((unsigned char*)&rand_result, 4);
    out[i] = rand_result % 10000;
  }
  return out;
}

void print_arr(const int* arr, const int len)
{
  int i;
  for (i = 0; i < len; ++i) EPrintf("%d ", arr[i]);
  EPrintf("\n");
}

int sort(int len)
{
  // int32_t* a = random_arr(len);
  int32_t a[10] = {99, 96, 1, 2, 3, 100, 56, 24, 78, 21};
  print_arr(a, len);
  int32_t* b = new int32_t[len];
  merge_sort(a, b, len);
  print_arr(b, len);
  // delete[] a;
  delete[] b;
  return 0;
}
