#include "./Enclave_t.h"
#include "./Enclave.h"
#include "./utility.h"
#include "./quick_sort_tx.h"
#include "./RealLibCoda.h"
int partition(HANDLE aData,int begin, int end) {
  int res = begin - 1;
  int pivot = nob_read_at(aData,end);
  int tmp = 0;
  for(int i=begin;i<=end-1;i++) {
    int v = nob_read_at(aData,i);
    if(v<pivot) {
        res++;
        tmp = nob_read_at(aData,res);
        nob_write_at(aData,res,nob_read_at(aData,i));
        nob_write_at(aData,i,tmp);
    }
  }
  tmp = nob_read_at(aData,end);
  nob_write_at(aData,end,nob_read_at(aData,res+1));
  nob_write_at(aData,res+1,tmp);
  return res+1;
}

void quick_sort_tx(int arr[], int begin, int end) {
  int stack[end-begin+1];
  HANDLE aStack = initialize_nob_array(stack,end-begin+1);
  HANDLE aData = initialize_nob_array(arr,end-begin+1);
  int top = -1;
  coda_txbegin();
  nob_write_at(aStack,++top,begin);
  nob_write_at(aStack,++top,end);
  while (top>=0) {
      end = nob_read_at(aStack,top--);
      begin = nob_read_at(aStack,top--);
      int p = partition(aData,begin,end);
      if (p-1 > begin) {
        nob_write_at(aStack,++top,begin);
        nob_write_at(aStack,++top,p-1);
      }
      if (p+1 < end) {
        nob_write_at(aStack,++top,p+1);
        nob_write_at(aStack,++top,end);
      }
  }
  coda_txend();
}

void quick_sort_tx_test() {
  int a[] = {9,8,7,6,5,4,3,2,1};
  quick_sort_tx(a,0,9);
  for(int i=0;i<9;i++)
    EPrintf("%d ",a[i]);
  EPrintf("\n");
}
