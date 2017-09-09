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

  int asm_compute_CPU();
  //int _abort_handler();
  int asm_tx_begin(int32_t* data, int32_t size1, int32_t size2);
  void asm_tx_end();
  void tx_abort(int code){
    //TODO
    bar1("tx aborted reason=%d\n",code);
  }

/*
static 
inline void tx_begin2(int32_t** data, int32_t* sizes, int32_t size) {
  bar1("xbegin, %p,%d\n",data,size);
  for (int j=0;j<size;j++){
    for (int i = 0; i < sizes[j]; i++)
      data[j][i]=(data[j][i] + 1) - 1;//TODO check dirty lines?
  }
  __asm__("xbegin _abort_handler");
  for (int j=0;j<size;j++){
    for (int i = 0; i < sizes[j]; i++)
      data[j][i]=(data[j][i] + 1) - 1;//TODO check dirty lines?
  }
}
*/
/*
static 
inline void tx_begin(int32_t* data, int32_t size) {
  //for (int j=0;j<size;j++)
    //  data[j]=5;//data[j]+1-1; //TODO check if causing dirty lines?
asm volatile("mov %0, %%edx\n\t"
        "mov $0, %%eax\n\t"
        "mov %1, %%rbx\n\t"
        "mov %%rbx,%%rcx\n\t"
        "loop1:cmpl  %%edx,%%eax\n\t"
        "jge    endloop1\n\t"
        "movl   $5, (%%rcx)\n\t"
        "addl   $1, %%eax\n\t"
        "add   $4, %%rcx\n\t"
        "jmp    loop1\n\t"
        "endloop1:\n\t"
        "xbegin _abort_handler\n\t"
        "mov %%rbx,%%rcx\n\t"
        "loop:cmpl  %%edx,%%eax\n\t"
        "jge    endloop\n\t"
        "movl   $5, (%%rcx)\n\t"
        "addl   $1, %%eax\n\t"
        "add   $4, %%rcx\n\t"
        "jmp    loop\n\t"
        "endloop:\n\t"
        :
        :"r"(size),"r"(data)
        :"rcx","eax","edx");
}
static 
inline void tx_end() {
  __asm__("xend");
  bar1("xend, %d\n",BLOWUPFACTOR);
}
*/
}

void compute_CPU3_distribute(int32_t* E_data, int32_t* E_perm, int32_t* E_output){
  bar1("BLOWUPFACTOR=%d\n",BLOWUPFACTOR);
  int32_t inter1[SqrtN*BLOWUPFACTOR+SqrtN+SqrtN];
  int32_t inter2[N*BLOWUPFACTOR];
  int32_t* E_data_prime = &inter1[SqrtN*BLOWUPFACTOR];
  int32_t* E_perm_prime = &inter1[SqrtN*BLOWUPFACTOR+SqrtN];
  for (int i=0;i<SqrtN;i++)
      E_data_prime[i] = i;
  for (int i=0;i<SqrtN;i++)
     // E_perm_prime[i] = SqrtN-1-i;
      E_perm_prime[i] = i;//SqrtN-1-i;
//  for (int i=0;i<SqrtN*BLOWUPFACTOR+2*SqrtN;i++)
//      bar1("@%d=%d\n",i,inter1[i]);
  for (int j=0; j<SqrtN; j++){
    int ret = asm_tx_begin(inter1,SqrtN*BLOWUPFACTOR,SqrtN);

/**
  int32_t inter1[SqrtN*BLOWUPFACTOR];
  int32_t inter2[N*BLOWUPFACTOR];
  int32_t* data[1]; data[0] = inter1;
  int32_t sizes[1]; sizes[0] = SqrtN*BLOWUPFACTOR;
  for (int j=0; j<SqrtN; j++){
    tx_begin2(data, sizes, 1);
*/
    //for (int i=0;i<SqrtN;i++)
      //inter1[E_perm_prime[i]] = E_data_prime[i];
    //asm_tx_end();
    //inter1 -> inter2
    //bar1("inter1=%d\n",inter1[SqrtN*BLOWUPFACTOR-1]);
  }
  return;
}

void compute_CPU(int32_t* E_data, int32_t* E_perm, int32_t* E_output){
  int ret = asm_compute_CPU();
  if (ret){
    bar1("transacation aborted!!\n",ret);
  }
}

void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N];
/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref)
{
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;

  copy_E_M(M_data, M_perm, E_data, E_perm);
//  compute_CPU(E_data, E_perm, E_output);
  compute_CPU3_distribute(E_data, E_perm, E_output);
//  for(int i=0;i<32;i++)
//    bar1("calling ocall_bar with: %d\n",E_output[i]);
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
