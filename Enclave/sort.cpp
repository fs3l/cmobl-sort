/*
  Author: Cheng Xu
  Integration: Ju Chen
  10/04/2017
*/

#include <math.h>
#include <sgx_trts.h>
#include <stdio.h>
#include <stdlib.h>

#include "./BetterLibCoda.h"
#include "./DummyLibCoda.h"
#include "./Enclave.h"
#include "./sort.h"

#define C 4

void print_arr(const int* arr, const int len);

class Queue
{
public:
  Queue(const int32_t max_len, Coda* coda)
  {
    data = new int32_t[max_len + 3];
    data[0] = max_len;
    data[1] = 0;
    data[2] = 0;
    nob = coda->make_nob_array(data, max_len + 3);
  }
  ~Queue()
  {
    delete nob;
    delete[] data;
  }

  __attribute__((always_inline)) inline int32_t get_max_len() const
  {
    return nob->read_at(0);
  }
  __attribute__((always_inline)) inline int32_t get_cur_len() const
  {
    return nob->read_at(1);
  }
  __attribute__((always_inline)) inline int32_t get_front_idx() const
  {
    return nob->read_at(2);
  }
  __attribute__((always_inline)) inline void set_cur_len(int32_t cur_len)
  {
    nob->write_at(1, cur_len);
  }
  __attribute__((always_inline)) inline void set_front_idx(int32_t front_idx)
  {
    nob->write_at(2, front_idx);
  }

  __attribute__((always_inline)) inline void enqueue(const int32_t val)
  {
    int32_t max_len = get_max_len();
    int32_t cur_len = get_cur_len();
    int32_t front_idx = get_front_idx();
    if (cur_len == max_len) abort();
    nob->write_at((front_idx + cur_len) % max_len + 3, val);
    ++cur_len;
    set_cur_len(cur_len);
  }
  __attribute__((always_inline)) inline int32_t front() const
  {
    int32_t cur_len = get_cur_len();
    int32_t front_idx = get_front_idx();
    if (cur_len == 0) abort();
    return nob->read_at(front_idx + 3);
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
  NobArray* nob;
};

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

  // TODO: replace new Coda() to proper implementation.
  Coda* coda = new DummyCoda();

  ObIterator* lhs_out_ob = coda->make_ob_iterator(lhs_out, lhs_len);
  ObIterator* rhs_out_ob = coda->make_ob_iterator(rhs_out, rhs_len);
  ObIterator* arr_out_ob = coda->make_ob_iterator(arr_out, len);

  size_t s = C * (int)sqrt((double)len);
  size_t half_s = s / 2;
  Queue lhs_q(s, coda), rhs_q(s, coda);

  size_t i, j;
  int32_t lhs_val, rhs_val, out_val;

  coda->tx_begin();

  for (i = 0; i < half_s; ++i) {
    if (i < lhs_len) {
      lhs_q.enqueue(lhs_out_ob->read_next());
    }
    if (i < rhs_len) {
      rhs_q.enqueue(rhs_out_ob->read_next());
    }
  }
  for (; i < rhs_len + half_s; ++i) {
    if (i < lhs_len) {
      lhs_q.enqueue(lhs_out_ob->read_next());
    }
    if (i < rhs_len) {
      rhs_q.enqueue(rhs_out_ob->read_next());
    }

    for (j = 0; j < 2; ++j) {
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
        arr_out_ob->write_next(out_val);
      } else {
        if (i < lhs_len || i < rhs_len) abort();
        goto done;
      }
    }
  }

  if (lhs_q.get_cur_len() > 0 && rhs_q.get_cur_len() > 0) abort();

done:

  while (lhs_q.get_cur_len() > 0) {
    out_val = lhs_q.front();
    lhs_q.dequeue();
    arr_out_ob->write_next(out_val);
  }

  while (rhs_q.get_cur_len() > 0) {
    out_val = rhs_q.front();
    rhs_q.dequeue();
    arr_out_ob->write_next(out_val);
  }

  coda->tx_end();

  delete lhs_out_ob;
  delete rhs_out_ob;
  delete arr_out_ob;

  delete[] lhs_out;
  delete[] rhs_out;

  delete coda;
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
  int32_t* a = random_arr(len);
  print_arr(a, len);
  int32_t* b = new int32_t[len];
  merge_sort(a, b, len);
  print_arr(b, len);
  delete[] a;
  delete[] b;
  return 0;
}
