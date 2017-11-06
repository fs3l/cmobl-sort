#include "./Enclave_t.h"
#include "./Enclave.h"
#include <math.h>
int record[6] = {100,4,101,1,102,2};
int counter[3] = {0,0,0};
int output[100];
int op = 0;
void write_output(int r, int count) {
  for (int i=0;i<count;i++) {
    output[op] = r;
    op++;
  }
}

int findMin() {
  int min = 1000000;
  int res = -1;
  for(int i=0;i<3;i++) {
    if (counter[i] !=0 && counter[i] < min) {
      min = counter[i];
      res = i;
    }
  }
  return res+100;
}

void expand() {
  int sum_weight=0;
  float avg_weight;
  int weight_current;
  for(int i=0;i<3;i++)
    sum_weight += record[i*2+1];
  avg_weight = sum_weight/3.0f;
  for(int i=0;i<3;i++) {
    weight_current = (int)(roundf((avg_weight)*(i+1))) - (int)(roundf(avg_weight*i));
    EPrintf("weight_current=%d\n",weight_current);
    if (record[i*2+1] <= weight_current) {
      write_output(record[i*2],record[i*2+1]);
      weight_current = weight_current - record[i*2+1];
    } else {
      counter[record[i*2]-100] = record[i*2+1];
    }
    while (weight_current>0) {
      int record_j = findMin();
      if (counter[record_j-100]>weight_current) {
        write_output(record_j,weight_current);
        counter[record_j-100] = counter[record_j-100] - weight_current;
        weight_current = 0;
      } else {
        write_output(record_j,counter[record_j-100]);
        weight_current -= counter[record_j-100];
        counter[record_j-100] = 0;
      }
    }
  }
  for(int i=0;i<op;i++)
    EPrintf("output[%d]=%d\n",i,output[i]);
}
