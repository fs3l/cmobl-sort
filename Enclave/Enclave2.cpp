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
  memcpy(E_data, M_data, N*sizeof(int32_t));
  memcpy(E_perm, M_perm, N*sizeof(int32_t));
  //TODO
}

extern "C" {
  int asm_apptx_distribute(int32_t* data, int32_t size1, int32_t size2);
  int asm_cache_hit_simulate(int32_t* data, int32_t size1);
  int asm_cache_miss_simulate(int32_t* data,int32_t size1);

static inline 
  void txbegin(int32_t* txmem, int32_t size){}

static inline 
  void txend(){
    //__asm__("xend");
    bar1("xend, %d\n",BLOWUPFACTOR);
  }
  void tx_abort(int code){
    //TODO
    static int i=0;
    i++;
    bar1("tx aborted count=%d\n",i);
  }
}

void applogic_distribute(int32_t* txmem, int32_t size){
   // int tmp[1000];// = new int[1000];
   // asm_cache_miss_simulate(tmp,1000);
    int ret = asm_apptx_distribute(txmem,SqrtN*BLOWUPFACTOR,SqrtN);
  return;
}

void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N];

void
memsetup(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t M_perm_size, int32_t* M_output, int32_t M_output_init, int32_t M_output_size, int32_t** txmem_p, int32_t* size_p){
//TODO gen. value by simulation
  bar1("BLOWUPFACTOR=%d\n",BLOWUPFACTOR);
  static int32_t txmem[SqrtN*BLOWUPFACTOR+SqrtN+SqrtN];
  int32_t* E_data_prime = &txmem[SqrtN*BLOWUPFACTOR];
  int32_t* E_perm_prime = &txmem[SqrtN*BLOWUPFACTOR+SqrtN];
  for (int i=0;i<SqrtN;i++)
      E_data_prime[i] = i;
  for (int i=0;i<SqrtN;i++)
     // E_perm_prime[i] = SqrtN-1-i;
      E_perm_prime[i] = i;//SqrtN-1-i;
//  copy_E_M(M_data, M_perm, E_data, E_perm);
  * txmem_p = txmem;
  * size_p = 2*SqrtN + SqrtN* BLOWUPFACTOR;
}

void apptx_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t M_perm_size, int32_t* M_output, int32_t M_output_init, int32_t M_output_size){
    int32_t* txmem;
    int32_t txmem_size;
    memsetup(M_data, M_data_init, M_data_size, M_perm, M_perm_init, M_perm_size, M_output, M_output_init, M_output_size, &txmem, &txmem_size);
    txbegin(txmem, txmem_size);
    applogic_distribute(txmem, txmem_size);
    txend();
}

/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref)
{
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;
  for (int i = 0; i < SqrtN; i++){
    apptx_distribute(M_data,i,SqrtN,M_perm,i,SqrtN,M_output,i,SqrtN*BLOWUPFACTOR);
  }
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
