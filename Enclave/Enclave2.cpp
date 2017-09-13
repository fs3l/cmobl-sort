#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <string.h>
#include <sgx_cpuid.h>

#include "sgx_trts.h"
#include "Enclave.h"
#include "Enclave_t.h"  /* bar*/

#include <setjmp.h>

jmp_buf bufferA;
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

  __attribute__ ((always_inline)) 
    void txbegin(int32_t* txmem, int32_t M_output_size, int32_t M_data_size){
      __asm__("lea (%%rip),%%r14\n\t"
          "movl %0,%%esi\n\t"
          "mov %1,%%rdi\n\t"
          "movl %2,%%edx\n\t"
          :
          :"r"(M_output_size),"r"(txmem),"r"(M_data_size)
          :"esi","edi","edx","r14");
      __asm__("movl %%esi, %%r8d\n\t"
          "mov $0, %%eax\n\t"
           "mov %%rdi, %%rcx\n\t"
          "loop_ep_%=:\n\t"
          "cmpl  %%r8d,%%eax\n\t"//  #r8d is the LSB of the r8
          "jge    endloop_ep_%=\n\t"
          "movl   $5, (%%rcx)\n\t"
          "addl   $1, %%eax\n\t"
          "add    $4, %%rcx\n\t"
          "jmp    loop_ep_%=\n\t"
          "endloop_ep_%=:\n\t"
          "xbegin asm_abort_handler\n\t"
          "mov %%rdi,%%rcx\n\t"
          "mov $0, %%eax\n\t"
          "loop_ip_%=:\n\t"
          "cmpl  %%r8d,%%eax\n\t"
          "jge    endloop_ip_%=\n\t"
          "movl   $5, (%%rcx)\n\t"
          "addl   $1, %%eax\n\t"
          "add   $4, %%rcx\n\t"
          "jmp    loop_ip_%=\n\t"
          "endloop_ip_%=:\n\t"
         :::);
    }

  __attribute__ ((always_inline)) void txend() {
    __asm__ ("xend");
    bar1("xend, %d\n",BLOWUPFACTOR);
  }
  void tx_abort(int code){
    //TODO
    static int i=0;
    i++;
    bar1("tx aborted count=%d\n",i);
  }
}
__attribute__ ((always_inline)) void app_distribute(){
    __asm__( "mov %%rdi,%%r8\n\t"  //r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"  // r12=4*size1
        "add %%r12,%%r8\n\t"   //r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
        "mov %%rdx,%%r12\n\t"  //r12 = size2
        "sall $2,%%r12d\n\t"  //r12=r12*4
        "add %%r12,%%r9\n\t"  //r9 is now permu
        "mov $0,%%eax\n\t"  //eax = 0
        "loop_dis_%=:\n\t"
        "cmpl %%edx,%%eax\n\t"
        "jge endloop_dis_%=\n\t"
        "movl (%%r8),%%r10d\n\t"  //data[i] -> r10
        "movl (%%r9),%%r11d\n\t"  //permu[i] -> r11
        "sall $2,%%r11d\n\t"     //r11 = r11 *4
        "mov %%rdi,%%rcx\n\t"  //rcx = txmem
        "add  %%r11,%%rcx\n\t"    //rcx is now output[permu[i]]
        "movl %%r10d,(%%rcx)\n\t" //assign value
        "addl $1,%%eax\n\t"  //increment
        "add $4,%%r8\n\t"   //increment
        "add $4,%%r9\n\t"  //increment
        "jmp loop_dis_%=\n\t"
        "endloop_dis_%=:\n\t":::);
  }

void applogic_distribute(int32_t* txmem, int32_t size){
   // int tmp[1000];// = new int[1000];
   // asm_cache_miss_simulate(tmp,1000);
   // int ret = asm_apptx_distribute(txmem,SqrtN*BLOWUPFACTOR,SqrtN);
  return;
}

void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N];

void
memsetup(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size, int32_t** txmem_p, int32_t* size_p){
//TODO gen. value by simulation
  bar1("BLOWUPFACTOR=%d\n",BLOWUPFACTOR);
  static int32_t txmem[SqrtN*BLOWUPFACTOR+SqrtN+SqrtN];
  int32_t* E_data_prime = &txmem[SqrtN*BLOWUPFACTOR];
  int32_t* E_perm_prime = &txmem[SqrtN*BLOWUPFACTOR+SqrtN];
  for (int i=0;i<SqrtN;i++)
      E_data_prime[i] = i;
  for (int i=0;i<SqrtN;i++)
      E_perm_prime[i] = SqrtN-1-i;
     // E_perm_prime[i] = i;//SqrtN-1-i;
//  copy_E_M(M_data, M_perm, E_data, E_perm);
  * txmem_p = txmem;
  * size_p = 2*SqrtN + SqrtN* BLOWUPFACTOR;
}

void apptx_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size){
    int32_t* txmem;
    int32_t txmem_size;
    memsetup(M_data, M_data_init, M_data_size, M_perm, M_perm_init, M_output, M_output_init, M_output_size, &txmem, &txmem_size);
    txbegin(txmem, M_output_size, M_data_size);
//    applogic_distribute(txmem, txmem_size);
    app_distribute();
    txend();
//    for (int i=0;i<M_data_size;i++)
//      bar1("txmem[%d]=%d\n",i,txmem[i]);
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
    apptx_distribute(M_data,i,SqrtN,M_perm,i,M_output,i,SqrtN*BLOWUPFACTOR);
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
