#include "./Enclave.h"
#include "./utility.h"

void swap(int* a, int* b)
{
  int temp = *a;
  *a = *b;
  *b = temp;
}

void print_arr(const int32_t* arr, int32_t len)
{
  for (int32_t i = 0; i < len; ++i) EPrintf("%d ", arr[i]);
  EPrintf("\n");
}
