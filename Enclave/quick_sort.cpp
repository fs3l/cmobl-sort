#include "./Enclave_t.h"
#include "./Enclave.h"
#include "./utility.h"
#include "./quick_sort.h"
int stack[10000000];
int partition(int arr[],int begin, int end) {
  int res = begin - 1;
  int pivot = arr[end];
  int tmp = 0;
  for(int i=begin;i<=end-1;i++) {
    if(arr[i]<pivot) {
        res++;
        swap(&arr[i],&arr[res]);
    }
  }
  swap(&arr[res+1],&arr[end]);
  return res+1;
}

void quick_sort(int arr[], int begin, int end) {
  int top = -1;
  int high_use = -1;
  stack[++top] = begin;
  stack[++top] = end;
  while (top>=0) {
      end = stack[top--];
      begin = stack[top--];
      int p = partition(arr,begin,end);
      if (p-1 > begin) {
        stack[++top] = begin;
        stack[++top] = p-1;
      }
      if (p+1 < end) {
        stack[++top] = p+1;
        stack[++top] = end;
      }
      if (top>high_use)
        high_use = top;
  }
  EPrintf("high_use=%d\n",high_use);
}

void quick_sort_test(int* arr, int n) {
  quick_sort(arr,0,n-1);
}
