#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <string.h>
#include <sgx_cpuid.h>

#include "sgx_trts.h"
#include "Enclave.h"
#include "Enclave_t.h"  /* bar*/

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int bar1(const char *fmt, ...)
{
  int ret[1];
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_bar(buf, ret);
  return ret[0];
}

void copy_E_M(int32_t* host_data, int32_t* host_perm, 
    int32_t* enc_data,
    int32_t* enc_perm){
  memcpy(enc_data, host_data, N*sizeof(int32_t));
  memcpy(enc_perm, host_perm, N*sizeof(int32_t));
  //TODO
}

void compute_CPU(int32_t* enc_data, int32_t* enc_perm, int32_t* enc_output){
  //copy_CPU_E(void)

  //copy_CPU_multiply(void)

  //copy_E_CPU(void)
}

void copy_M_E(int32_t* enc_output, int32_t* host_output){
  memcpy(host_output,enc_output,BLOWUPFACTOR*N*sizeof(int32_t));
}

int32_t enc_data[N];
int32_t enc_perm[N];
int32_t enc_output[BLOWUPFACTOR*N];

/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long host_data_ref, long host_perm_ref, long host_output_ref)
{
  void *ptr = malloc(100);
  assert(ptr != NULL);
  memset(ptr, 0x0, 100);
  free(ptr);

  int32_t* host_data = (int32_t*)host_data_ref;
  int32_t* host_perm = (int32_t*)host_perm_ref;
  int32_t* host_output = (int32_t*)host_output_ref;

  int ret = bar1("calling ocall_bar with: %d\n", host_data[3]);

  copy_E_M(host_data, host_perm, enc_data, enc_perm);
  bar1("calling ocall_bar with: %d\n", host_data[3]);
  compute_CPU(enc_data, enc_perm, host_output);
  bar1("calling ocall_bar with: %d\n", host_data[3]);
  copy_M_E(enc_output, host_output);
  bar1("calling ocall_bar with: %d\n", host_data[3]);
  return 0;
}

/* ecall_sgx_cpuid:
 *   Uses sgx_cpuid to get CPU features and types.
 */
void ecall_sgx_cpuid(int cpuinfo[4], int leaf)
{
  sgx_status_t ret = sgx_cpuid(cpuinfo, leaf);
  if (ret != SGX_SUCCESS)
    abort();
}
