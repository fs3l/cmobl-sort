#ifndef _SORT_H
#define _SORT_H

#ifdef __cplusplus
extern "C" {
#endif

void merge_sort(const int32_t* arr_in, int32_t* arr_out, const size_t len);

void merge_sort_test();

#ifdef __cplusplus
}
#endif

#endif
