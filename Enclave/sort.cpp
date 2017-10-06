/*
  Author: Cheng Xu
  Integration: Ju Chen
  10/04/2017
*/

#include <math.h>
#include <sgx_trts.h>
#include <stdio.h>
#include <stdlib.h>

#include "./sort.h"
#include "./Enclave.h"

#define C 4

void print_arr(const int* arr, const int len);

struct queue_s {
    int* data;
    int front_idx;
    int cur_len;
    int max_len;
};
typedef struct queue_s queue_t;
typedef struct queue_s* queue_ptr;

queue_ptr q_init(const int max_len)
{
    queue_ptr q;
    q = (queue_ptr)malloc(sizeof(queue_t));
    q->data = (int*)malloc(max_len * sizeof(int));
    q->front_idx = 0;
    q->cur_len = 0;
    q->max_len = max_len;
    return q;
}

void q_free(queue_ptr q)
{
    free(q->data);
    free(q);
}

void q_enqueue(queue_ptr q, const int val)
{
    if (q->cur_len == q->max_len) abort();
    q->data[(q->front_idx + q->cur_len) % q->max_len] = val;
    ++q->cur_len;
}

int q_front(const queue_ptr q)
{
    if (q->cur_len == 0) abort();
    return q->data[q->front_idx];
}

int q_dequeue(queue_ptr q)
{
    if (q->cur_len == 0) abort();
    int val = q->data[q->front_idx];
    --q->cur_len;
    q->front_idx = (q->front_idx + 1) % q->max_len;
    return val;
}

void merge_sort(const int* arr_in, int* arr_out, const int len)
{
    if (len == 1) {
        arr_out[0] = arr_in[0];
        return;
    }

    int lhs_len = len / 2;
    int rhs_len = len - lhs_len;
    int* lhs_out = (int*)malloc(lhs_len * sizeof(int));
    int* rhs_out = (int*)malloc(rhs_len * sizeof(int));
    merge_sort(arr_in, lhs_out, lhs_len);
    merge_sort(arr_in + lhs_len, rhs_out, rhs_len);

    int s = C * (int)sqrt((double)len);
    int half_s = s / 2;
    queue_ptr lhs_q = q_init(s);
    queue_ptr rhs_q = q_init(s);

    int i;
    for (i = 0; i < half_s; ++i) {
        if (i < lhs_len) {
            EPrintf("read lhs[%d]\n", i);
            q_enqueue(lhs_q, lhs_out[i]);
        }
        if (i < rhs_len) {
            EPrintf("read rhs[%d]\n", i);
            q_enqueue(rhs_q, rhs_out[i]);
        }
    }
    int out_idx = 0;
    for (; i < rhs_len + half_s; ++i) {
        if (i < lhs_len) {
            EPrintf("read lhs[%d]\n", i);
            q_enqueue(lhs_q, lhs_out[i]);
        }
        if (i < rhs_len) {
            EPrintf("read rhs[%d]\n", i);
            q_enqueue(rhs_q, rhs_out[i]);
        }

        int lhs_val, rhs_val, out_val;

        for (int j = 0; j < 2; ++j) {
            if (lhs_q->cur_len > 0 && rhs_q->cur_len > 0) {
                lhs_val = q_front(lhs_q);
                rhs_val = q_front(rhs_q);
                if (lhs_val < rhs_val)
                    out_val = q_dequeue(lhs_q);
                else if (lhs_val == rhs_val) {
                    if (lhs_q->cur_len > rhs_q->cur_len)
                        out_val = q_dequeue(lhs_q);
                    else
                        out_val = q_dequeue(rhs_q);
                } else
                    out_val = q_dequeue(rhs_q);
                EPrintf("write arr_out[%d]\n", out_idx);
                arr_out[out_idx++] = out_val;
            } else {
                if (i < lhs_len || i < rhs_len) abort();
            }
        }
    }

    if (lhs_q->cur_len > 0 && rhs_q->cur_len > 0) abort();

    while (lhs_q->cur_len > 0) {
        EPrintf("write arr_out[%d]\n", out_idx);
        arr_out[out_idx++] = q_dequeue(lhs_q);
    }

    while (rhs_q->cur_len > 0) {
        EPrintf("write arr_out[%d]\n", out_idx);
        arr_out[out_idx++] = q_dequeue(rhs_q);
    }

    free(lhs_out);
    free(rhs_out);
    q_free(lhs_q);
    q_free(rhs_q);
}

int* init_arr(const int len) { return (int*)malloc(sizeof(int) * len); }
int* random_arr(const int len)
{
    unsigned int rand_result;
    sgx_read_rand((unsigned char*)&rand_result, 4);
    int* out = init_arr(len);
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
    int* a = random_arr(len);
    print_arr(a, len);
    int* b = init_arr(len);
    merge_sort(a, b, len);
    print_arr(b, len);
    return 0;
}
