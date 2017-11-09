#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>

void swap(int* a, int* b);
void swap_by_dir(int* a, int* b, int dir);
void print_arr(const int32_t* arr, int32_t len);

__attribute__((always_inline)) inline int32_t min(int32_t a, int32_t b)
{
  return a < b ? a : b;
}

__attribute__((always_inline)) inline int32_t max(int32_t a, int32_t b)
{
  return a > b ? a : b;
}

#endif
