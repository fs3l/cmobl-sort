#include "./Enclave_t.h"
#include "./Enclave.h"
#include "./RealLibCoda.h"
#include <math.h>
int record[6] = {100,4,101,1,102,2};
int counter[3] = {0,0,0};
int output[20];
int op = 0;
HANDLE iterRecord;
HANDLE iterOutput;
HANDLE arrayCounter;
__attribute__((always_inline)) inline void write_output(int r, int count) {
  for (int i=0;i<count;i++) {
    ob_rw_write_next(iterOutput,r);
  }
}

__attribute__ ((always_inline)) inline int findMin() {
  int min = 1000000;
  int res = -1;
  int cur;
  for(int i=0;i<3;i++) {
    cur = nob_read_at(arrayCounter,i);
    if (cur !=0 && cur < min) {
      min = cur;
      res = i;
    }
  }
  return res+100;
}

void expand() {
  int sum_weight=0;
  float avg_weight;
  int current_weight;
  int this_record;
  int this_weight;
  for(int i=0;i<3;i++)
    sum_weight += record[i*2+1];
  avg_weight = sum_weight/3.0f;

  iterRecord = initialize_ob_iterator(record,6);
  iterOutput = initialize_ob_rw_iterator(output,20);
  arrayCounter = initialize_nob_array(counter,3);
  coda_txbegin();
  for(int i=0;i<3;i++) {
    current_weight = (int)(roundf((avg_weight)*(i+1))) - (int)(roundf(avg_weight*i));
    this_record = ob_read_next(iterRecord);
    this_weight = ob_read_next(iterRecord);
    if (this_weight <= current_weight) {
      write_output(this_record,this_weight);
      current_weight = current_weight - this_weight;
    } else {
      nob_write_at(arrayCounter,this_record-100,this_weight);
    }
    while (current_weight>0) {
      int record_j = findMin();
      int min_count;
      min_count = nob_read_at(arrayCounter,record_j-100);
      if (min_count>current_weight) {
        write_output(record_j,current_weight);
        min_count -= current_weight;
        nob_write_at(arrayCounter,record_j-100,min_count);
        current_weight = 0;
      } else {
        write_output(record_j,min_count);
        current_weight -= min_count;
        nob_write_at(arrayCounter,record_j-100,0);
      }
    }
  }
  coda_txend();
  for(int i=0;i<7;i++)
    EPrintf("output[%d]=%d\n",i,ob_rw_read_next(iterOutput));
}
