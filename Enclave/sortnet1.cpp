#include "./Enclave.h"
#include "./Enclave_t.h"
#include "./utility.h"
#include "./sortnet1.h"
#include "./quick_sort.h"
void bitonicMerge(int lo, int cnt, int dir, int* list) {
  if (cnt>1) {
    int k=cnt/2;
    int i;
    for (i=lo; i<lo+k; i++) {
      swap_by_dir(&list[i],&list[i+k],dir);
    }
    bitonicMerge(lo, k, dir, list);
    bitonicMerge(lo+k, k, dir, list);
  }
}

void bitonicSort(int lo, int cnt, int dir, int* list) {
 // if (cnt<=16777216) {
 //   quick_sort_test(list+lo,cnt);
 // } else {
    if (cnt>1) {
    int k=cnt/2;
    bitonicSort(lo, k, 1, list);
    bitonicSort(lo+k, k, 0, list);
    bitonicMerge(lo, cnt, dir, list);
  }
}

void sortnet1_test(int* arr, int n) {
  bitonicSort(0,n,1,arr);
}

