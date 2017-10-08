#include <stdarg.h>
#include <stdio.h> /* vsnprintf */

#include <sgx_cpuid.h>
#include <sgx_trts.h>
#include <string.h>

#include "./Enclave.h"
#include "./Enclave_t.h" /* bar*/
#include "./melshuffle.h"
#include "./sort.h"
int coda_shuffle();
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

/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_shuffle(long M_data_ref, long M_perm_ref, long M_output_ref,
                  int c_size)
{
  // melshuffle(M_data_ref, M_perm_ref, M_output_ref, c_size, N);
  sort(10);
  // coda_shuffle();
}

/* ecall_sgx_cpuid:
 *   Uses sgx_cpuid to get CPU features and types.
 */
void ecall_sgx_cpuid(int cpuinfo[4], int leaf)
{
  sgx_status_t ret = sgx_cpuid(cpuinfo, leaf);
  if (ret != SGX_SUCCESS) abort();
}
