#include <stdarg.h>
#include <stdio.h> /* vsnprintf */

#include <sgx_cpuid.h>
#include <sgx_trts.h>
#include <string.h>

#include "./Enclave.h"
#include "./Enclave_t.h" /* bar*/
#include "./cache_shuffle.h"
#include "./coda_melshuffle.h"
#include "./expansion.h"
#include "./mergesort.h"
#include "./obli_merge_sort.h"
#include "./quick_sort.h"
#include "./quick_sort_tx.h"
#include "./sortnet1.h"

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int EPrintf(const char *fmt, ...)
{
  int ret[1];
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_printf(buf, ret);
  return ret[0];
}

void Eabort(const char *fmt, ...)
{
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_abort(buf);
}

int ecall_shuffle(long M_data_ref, long M_perm_ref, long M_output_ref,
                  int c_size)
{
  //  int* res;
  //  int a[1024];
  //  for (int i=0;i<1024;i++)
  //    a[i] = 1024-i;
  // melshuffle(M_data_ref, M_perm_ref, M_output_ref, c_size, N);
  long sec_begin[1], sec_end[1], usec_begin[1], usec_end[1];
  ocall_gettimenow(sec_begin, usec_begin);
  cache_shuffle_test();
  // merge_sort_test();
  // quick_sort_test((int*)M_data_ref,SortN);
  //  sortnet1_test((int * )M_data_ref,SortN);
  //  coda_melshuffle(M_data_ref, M_perm_ref, M_output_ref, c_size,N);
  //  res= msort(a,1024);
  //  expand();
  ocall_gettimenow(sec_end, usec_end);
  EPrintf("time = %ld\n", (sec_end[0] * 1000000 + usec_end[0]) -
                              (sec_begin[0] * 1000000 + usec_begin[0]));
  //  for (int i=0;i<64;i++)
  //  EPrintf("res[%d]=%d\n",i,res[i]);
}

/* ecall_sgx_cpuid:
 *   Uses sgx_cpuid to get CPU features and types.
 */
void ecall_sgx_cpuid(int cpuinfo[4], int leaf)
{
  sgx_status_t ret = sgx_cpuid(cpuinfo, leaf);
  if (ret != SGX_SUCCESS) abort();
}
