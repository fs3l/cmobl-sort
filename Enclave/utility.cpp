#include "./Enclave.h"
#include "./utility.h"

void swap(int* a, int* b)
{
  int temp = *a;
  *a = *b;
  *b = temp;
}

void swap_by_dir(int* a, int* b,int dir)
{
  if ((dir==1 && *a > *b) || (dir==0 && *a < *b)) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
  }
}

void print_arr(const int32_t* arr, int32_t len)
{
  for (int32_t i = 0; i < len; ++i) EPrintf("%d ", arr[i]);
  EPrintf("\n");
}


void cmove_int32(bool cond, int32_t* src, int32_t* dest) {
  __asm__ (
      "mov %%rdi, %%rax\n\t"
      "mov (%%rsi),%%rbx\n\t"
      "cmove %%rbx,%%rcx\n\t"
      "mov %%rcx,(%%rdx)\n\t"
      :::
  );
}
