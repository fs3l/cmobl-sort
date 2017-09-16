#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <string.h>
#include <sgx_cpuid.h>

#include "sgx_trts.h"
#include "Enclave.h"
#include "Enclave_t.h"  /* bar*/

#include <setjmp.h>

uint64_t context[100];
int abort_count=0;
int start_count=0;
int finish_count=0;
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
  int asm_cache_hit_simulate(int32_t* data, int32_t size1);
  int asm_cache_miss_simulate(int32_t* data,int32_t size1);

  __attribute__ ((always_inline)) 
    void txbegin(int32_t* txmem, int32_t nob_size, int32_t ob_size){
      __asm__(
                 "movl %0,%%esi\n\t"
          "mov %1,%%rdi\n\t"
          "movl %2,%%edx\n\t"
          :
          :"r"(nob_size),"r"(txmem),"r"(ob_size)
          :"esi","edi","edx");
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
    finish_count++;
  }
  void tx_abort(int code){
    //TODO
    abort_count++;
  }
}

__attribute__ ((always_inline)) void app_merge(){
  /*
  int i = 0;
  int j= 0;
  int k= 0;
  while (i<stride && j<stride) {
    if (tx_mem[2*stride+i] < tx_mem[3*stride+j]) {tx_mem[k]=tx_mem[2*stride+i];i++;}
    else {tx_mem[k]=tx_mem[3*stride+j];j++;}
    k++;
  }
  while (i<stride) {tx_mem[k]=tx_mem[2*stride+i];i++;k++;}    
  while (j<stride) {tx_mem[k]=tx_mem[3*stride+j];j++;k++;}*/ 
  __asm__( "mov %%rdi,%%r8\n\t"  //r8 = txmem
      "mov %%rsi,%%r12\n\t"  // r12 = size1
      "sall $2,%%r12d\n\t"  // r12=4*size1
      "add %%r12,%%r8\n\t"   //r8 is src1
      "mov %%r8,%%r9\n\t"    // r9 = data
      "mov %%rdx,%%r12\n\t"  //r12 = size2
      "sall $2,%%r12d\n\t"  //r12=r12*4
      "add %%r12,%%r9\n\t"  //r9 is now src2
      "mov $0,%%eax\n\t"  //eax = i = 0
      "mov $0,%%ebx\n\t"  //ebx = j = 0
      "mov $0,%%ecx\n\t"  //ecx = k = 0
      "loop_dis_%=:\n\t"
      "cmpl %%edx,%%eax\n\t"
      "jge endloop_dis_%=\n\t"
      "cmpl %%edx,%%ebx\n\t"
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
int32_t E_output[N*BLOWUPFACTOR];

void 
memsetup_cleanup_msort(int32_t* data, int32_t data_init, int32_t data_size, int32_t** txmem_p, int32_t* size_p){
  static int32_t txmem[SqrtN*BLOWUPFACTOR];
  for(int i=0;i<SqrtN*BLOWUPFACTOR;i++)
      txmem[i] = data[(i/BLOWUPFACTOR)*BLOWUPFACTOR*SqrtN + i%BLOWUPFACTOR + data_init*BLOWUPFACTOR];
  *txmem_p = txmem;
  *size_p = SqrtN*BLOWUPFACTOR;
}



void
memsetup_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size, int32_t** txmem_p, int32_t* size_p){
  //TODO gen. value by simulation
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

void cas_plain(int* a,int* b, int dir) {
  __asm__(
      "mov (%rdi), %ecx\n\t"
      "mov	(%rsi), %ebx\n\t"
      "cmp	%ebx, %ecx\n\t"
      "setg %al\n\t"
      "movzb %al, %eax\n\t"
      "cmp %eax, %edx\n\t"
      "jne     out\n\t"
      "mov    %ebx, %eax\n\t"
      "mov    %ecx, %ebx\n\t"
      "mov    %eax, %ecx\n\t"
      "mov	%ecx, (%rdi)\n\t"
      "mov	%ebx, (%rsi)\n\t"
      "out:\n\t");
}

/*
   void cas_cmove(int *a, int *b) {
   __asm__(
   "mov (%rdi), %ecx\n\t"
   "mov	(%rsi), %ebx\n\t"
   "cmp	%ebx, %ecx\n\t"
   "setg %al\n\t"
   "movzb %al, %eax\n\t"
   "cmp %eax, %edx\n\t"
   "mov    %ebx, %eax\n\t"
   "mov    %ecx, %ebx\n\t"
   "mov    %eax, %ecx\n\t"
   "mov	%ecx, (%rdi)\n\t"
   "mov	%ebx, (%rsi)\n\t"
   "out:\n\t");
   }*/


void apptx_cleanup_bsort(int32_t* data, int32_t data_init, int32_t data_size){
  //TODO
  int i, j;
  for (i=0;i<data_size-1;i++)
    for (j=0;j<data_size-i-1;j++)
      cas_plain(&data[j],&data[j+1],1);
}

void do_merge(int32_t* dst, int32_t*src1,int32_t* src2,int stride) {
  int32_t tx_mem[4*stride];
  for(int p=0;p<stride;p++)
    tx_mem[p+2*stride] = src1[p];
  for(int q=0;q<stride;q++)
    tx_mem[q+3*stride] = src2[q];
  txbegin(tx_mem, 2*stride, stride);
  int i = 0;
  int j= 0;
  int k= 0;
  while (i<stride && j<stride) {
    if (tx_mem[2*stride+i] < tx_mem[3*stride+j]) {tx_mem[k]=tx_mem[2*stride+i];i++;}
    else {tx_mem[k]=tx_mem[3*stride+j];j++;}
    k++;
  }
  while (i<stride) {tx_mem[k]=tx_mem[2*stride+i];i++;k++;}    
  while (j<stride) {tx_mem[k]=tx_mem[3*stride+j];j++;k++;}    
  txend();
  for(int p=0;p<2*stride;p++)
    dst[p] = tx_mem[p];
}

int32_t*  apptx_cleanup_msort(int32_t* data, int32_t data_init, int32_t data_size){
  int32_t* txmem;
  int32_t txmem_size;
  //memsetup_cleanup_msort(data, data_init, data_size, &txmem, &txmem_size);
  int32_t* tmp_data = new int[data_size];
  int32_t* swap;
  for(int stride=1;stride<data_size;stride*=2) {
    for(int j=0; j<data_size; j+=2*stride)
      do_merge(tmp_data+j,data+j,data+j+stride,stride);
    swap = data;
    data = tmp_data;
    tmp_data = swap;
  }
  return data;
}

void apptx_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size){
  int32_t* txmem;
  int32_t txmem_size;
  memsetup_distribute(M_data, M_data_init, M_data_size, M_perm, M_perm_init, M_output, M_output_init, M_output_size, &txmem, &txmem_size);
  __asm__(   "mov %%rax,%0\n\t"
          "mov %%rbx,%1\n\t"
          "mov %%rcx,%2\n\t"
          "mov %%rdx,%3\n\t"
          "mov %%rdi,%4\n\t"
          "mov %%rsi,%5\n\t"
          "mov %%r8,%6\n\t"
          "mov %%r9,%7\n\t"
          "mov %%r10,%8\n\t"
          "mov %%r11,%9\n\t"
          "mov %%r12,%10\n\t"
          "mov %%r13,%11\n\t"
          "mov %%r15,%12\n\t"
          "lea (%%rip),%%r14\n\t"
          "mov %13,%%rax\n\t"
          "mov %14,%%rbx\n\t"
          "mov %15,%%rcx\n\t"
          "mov %16,%%rdx\n\t"
          "mov %17,%%rdi\n\t"
          "mov %18,%%rsi\n\t"
          "mov %19,%%r8\n\t"
          "mov %20,%%r9\n\t"
          "mov %21,%%r10\n\t"
          "mov %22,%%r11\n\t"
          "mov %23,%%r12\n\t"
          "mov %24,%%r13\n\t"
          "mov %25,%%r15\n\t"
          :"=r"(context[0]),"=r"(context[1]),"=r"(context[2]),"=r"(context[3]),"=r"(context[4]),
           "=r"(context[5]),"=r"(context[6]),"=r"(context[7]),"=r"(context[8]),"=r"(context[9]),
           "=r"(context[10]),"=r"(context[11]),"=r"(context[12])
          :"r"(context[0]),"r"(context[1]),"r"(context[2]),"r"(context[3]),"r"(context[4]),
           "r"(context[5]),"r"(context[6]),"r"(context[7]),"r"(context[8]),"r"(context[9]),
           "r"(context[10]),"r"(context[11]),"r"(context[12])
          :);
  start_count++;
  txbegin(txmem, M_output_size, M_data_size);
  //    applogic_distribute(txmem, txmem_size);
  app_distribute();
  txend();
  for (int i=0;i<M_data_size;i++)
    M_output[M_output_init*SqrtN*BLOWUPFACTOR+i] = txmem[i];
}

void testMergeSort() {
  int32_t data[8] = {8,7,6,5,4,3,2,1};
  int32_t* data1 = apptx_cleanup_msort(data,0,8);
  for(int i=0;i<8;i++)
    bar1("data1[%d]=%d\n",i,data1[i]);
}


/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size)
{
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;
  if (4*(2*SqrtN+SqrtN*BLOWUPFACTOR) > c_size) {
    return 0;
  }
  for (int i = 0; i < SqrtN; i++){
    apptx_distribute(M_data,i,SqrtN,M_perm,i,M_output,i,SqrtN*BLOWUPFACTOR);
  }
  bar1("start=%d,finish=%d,abort=%d\n",start_count,finish_count,abort_count);
  //for (int i=0;i<N*BLOWUPFACTOR;i++)
  //  bar1("M_putput[%d]=%d\n",i,M_output[i]); 
  /* unit test */
  //testMergeSort();  
  //for (int j = 0; j < SqrtN; j++){
  //  int32_t* ret = apptx_cleanup_msort(M_output,j,SqrtN*BLOWUPFACTOR);
  //  for(int i=0;i<SqrtN*BLOWUPFACTOR;i++)
  //    M_output[(i/BLOWUPFACTOR)*BLOWUPFACTOR*SqrtN + i%BLOWUPFACTOR + j*BLOWUPFACTOR] = ret[i];
 // }
 // for (int i=0;i<N*BLOWUPFACTOR;i++)
  //  bar1("M_putput[%d]=%d\n",i,M_output[i]); 

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
