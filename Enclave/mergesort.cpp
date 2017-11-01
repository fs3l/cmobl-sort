#include "./Enclave.h"
#include "./Enclave_t.h"
int tmp[1024];
void cpp_merge(int32_t* dst, int32_t* src1, int32_t* src2, int stride)
{
  // memsetup
  int i = 0;
  int j = 0;
  int k = 0;
  while (i < stride && j < stride) {
    if (src1[i] < src2[j]) {
      dst[k] = src1[i];
      i++;
    } else {
      dst[k] = src2[j];
      j++;
    }
    k++;
  }
  while (i < stride) {
    dst[k] = src1[i];
    i++;
    k++;
  }
  while (j < stride) {
    dst[k] = src2[j];
    j++;
    k++;
  }
}

int32_t* msort(int32_t* data, int32_t data_size)
{
  int32_t* input;
  int32_t* output;
  input = data;
  output = tmp;
  int32_t* swap;
  for (int stride = 1; stride < data_size; stride *= 2) {
    for (int j = 0; j < data_size; j += 2 * stride) {
      //  apptx_merge(output + j, input + j, input + j + stride, stride);
      cpp_merge(output + j, input + j, input + j + stride, stride);
    }
    swap = input;
    input = output;
    output = swap;
  }
  return input;
}
