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

void copy_E_M(int32_t* M_data, int32_t* M_perm, 
    int32_t* E_data,
    int32_t* E_perm){
  memcpy(E_data, M_data, M_N*sizeof(int32_t));
  memcpy(E_perm, M_perm, M_N*sizeof(int32_t));
  //TODO
}
extern "C" {
  int asm_compute_CPU();
}

void compute_CPU(int32_t* E_data, int32_t* E_perm, int32_t* E_output){
  int ret = asm_compute_CPU();
  if (!ret)
    bar1("transacation aborted!!\n",ret);
}

void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,BLOWUPFACTOR * M_N * sizeof(int32_t));
}

int32_t E_data[M_N];
int32_t E_perm[M_N];
int32_t E_output[BLOWUPFACTOR*M_N];
/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref)
{
  bar1("BLOWUPFACTOR=%d\n",BLOWUPFACTOR);
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;


  copy_E_M(M_data, M_perm, E_data, E_perm);
  //TOREMOVE: temporarily assign value for test purposes
  E_data[0] = 11;
  E_data[1] = 22;
  E_data[2] = 77;
  E_data[3] = 44;
  E_perm[0] = 12;
  E_perm[1] = 13;
  E_perm[2] = 14;
  E_perm[3] = 15;
  memset(E_output,-1,BLOWUPFACTOR*M_N*sizeof(int32_t));
  compute_CPU(E_data, E_perm, E_output);
  for(int i=0;i<32;i++)
  bar1("calling ocall_bar with: %d\n",E_output[i]);
  //TODO FIXME 
  // copy_M_E(E_output, M_output);
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
