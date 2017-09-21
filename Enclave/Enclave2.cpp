#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <string.h>
#include <sgx_cpuid.h>

#include "sgx_trts.h"
#include "Enclave.h"
#include "Enclave_t.h"  /* bar*/

#include <setjmp.h>

uint64_t gContext[100];
int g_aborts=0;
int g_starts=0;
int g_finishes=0;
int32_t g_scratch[2*BLOWUPFACTOR*N];
int32_t g_tx_mem[8192];

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
          "movl   (%%rcx),%%r11d\n\t"
          "movl   %%r11d, (%%rcx)\n\t"
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
          "movl   (%%rcx),%%r11d\n\t"
          "movl   %%r11d, (%%rcx)\n\t"
          "addl   $1, %%eax\n\t"
          "add   $4, %%rcx\n\t"
          "jmp    loop_ip_%=\n\t"
          "endloop_ip_%=:\n\t"
          :::);
    }

  __attribute__ ((always_inline)) void txend() {
    __asm__ ("xend");
    g_finishes++;
  }
  void tx_abort(int code){
    g_aborts++;
  }
}

__attribute__ ((always_inline)) void app_merge(){
  __asm__( "mov %%rdi,%%r8\n\t"  //r8 = txmem
      "mov %%rsi,%%r12\n\t"  // r12 = size1
      "sall $2,%%r12d\n\t"  // r12=4*size1
      "add %%r12,%%r8\n\t"   //r8 is src1
      "mov %%r8,%%r9\n\t"    // r9 = data
      "mov %%rdx,%%r12\n\t"  //r12 = size2
      "sall $2,%%r12d\n\t"  //r12=r12*4
      "add %%r12,%%r9\n\t"  //r9 is now src2
      "mov %%rdi,%%r12\n\t"  //r12 is now txmem
      "mov $0,%%eax\n\t"  //eax = i = 0
      "mov $0,%%ebx\n\t"  //ebx = j = 0
      "loop_merge_%=:\n\t"  
      "cmpl %%edx,%%eax\n\t"
      "jge endloop_merge_%=\n\t"
      "cmpl %%edx,%%ebx\n\t"
      "jge endloop_merge_%=\n\t"
      "movl (%%r8),%%r10d\n\t"  //src1[i] -> r10
      "movl (%%r9),%%r11d\n\t"  //src2[j] -> r11
      "movl %%r10d,(%%r12)\n\t" //assign value
      "movl 4(%%r8),%%r13d\n\t"  //src1[i+1] -> r13
      "movl %%r13d,4(%%r12)\n\t" //assign value
      "addl $2,%%eax\n\t"  //i++
      "add $8,%%r8\n\t"  //i++
      "cmpl %%r10d,%%r11d\n\t"
      "jge another_path_%=\n\t"     
      "movl %%r11d,(%%r12)\n\t" //assign value
      "movl 4(%%r9),%%r13d\n\t"  //src1[i+1] -> r13
      "movl %%r13d,4(%%r12)\n\t" //assign value
      "addl $2,%%ebx\n\t"  //j++
      "add $8,%%r9\n\t"  //j++
      "subl $2,%%eax\n\t"
      "sub $8,%%r8\n\t"
      "another_path_%=:\n\t"  
      "add $8,%%r12\n\t"   //k++
      "jmp loop_merge_%=\n\t"
      "endloop_merge_%=:\n\t"
      "loop_append1_%=:\n\t"  
      "cmpl %%edx,%%eax\n\t"
      "jge endloop_append1_%=\n\t"
      "movl (%%r8),%%r10d\n\t"  //src1[i] -> r10
      "movl %%r10d,(%%r12)\n\t" //assign value
      "movl 4(%%r8),%%r10d\n\t"  //src1[i] -> r10
      "movl %%r10d,4(%%r12)\n\t" //assign value
      "addl $2,%%eax\n\t"  //i++
      "add $8,%%r8\n\t"  //i++
      "add $8,%%r12\n\t"   //k++
      "jmp loop_append1_%=\n\t"
      "endloop_append1_%=:\n\t"
      "loop_append2_%=:\n\t"  
      "cmpl %%edx,%%ebx\n\t"
      "jge endloop_append2_%=\n\t"
      "movl (%%r9),%%r11d\n\t"  //src2[j] -> r11
      "movl %%r11d,(%%r12)\n\t" //assign value
      "movl 4(%%r9),%%r11d\n\t"  //src2[j] -> r11
      "movl %%r11d,4(%%r12)\n\t" //assign value
      "addl $2,%%ebx\n\t"  //j++
      "add $8,%%r9\n\t"  //j++
      "add $8,%%r12\n\t"   //k++
      "jmp loop_append2_%=\n\t"
      "endloop_append2_%=:\n\t":::);
}

/*
   data layout:

output:
pointer perm data perm data  ... ... perm data
.............................................
pointer perm data perm data  ... ... perm data 

data
data data ... ... data

perm
pos perm pos perm ... ... pos perm

 */


__attribute__ ((always_inline)) void app_distribute2(){
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
      "movl (%%r9),%%r11d\n\t"  //offset[i] -> r11
      "mov %%rdi,%%r10\n\t"  // r10 is now txmem
      "add %%r11,%%r10\n\t"  // r10 is now pointer address 
      "mov (%%r10),%%r11d\n\t"  // pos -> r11
      "mov %%r10,%%r12\n\t"  // r12  is now pointer adddress
      "add %%r11,%%r10\n\t"   // r10 is now target data address
      "add $4,%%r10\n\t"   // r10 is now target data address
      "movl (%%r8),%%r11d\n\t"  // r11 is data
      "movl %%r11d, 4(%%r10)\n\t"  //assign data 
      "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
      "movl %%r11d, (%%r10)\n\t"  //assign perm 
      "movl (%%r12),%%r11d\n\t"  // r11d is pointer address
      "addl $8,%%r11d\n\t"   //increment
      "movl %%r11d,(%%r12)\n\t"  // write offset back
      "addl $1,%%eax\n\t"  //increment
      "add $4,%%r8\n\t"   //increment
      "add $8,%%r9\n\t"  //increment
      "jmp loop_dis_%=\n\t"
      "endloop_dis_%=:\n\t":::);
}

void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N*BLOWUPFACTOR];


void 
memsetup_cleanup_msort(int32_t* data, int32_t data_init, int32_t data_size, int32_t** txmem_p, int32_t* size_p){
  static int32_t msortMem[2*SqrtN*BLOWUPFACTOR];
  for(int i=0;i<2*SqrtN*BLOWUPFACTOR;i++) 
    msortMem[i] = data[i/(2*BLOWUPFACTOR)*2*BLOWUPFACTOR*SqrtN + i%(2*BLOWUPFACTOR) + data_init*2*BLOWUPFACTOR];
  *txmem_p = msortMem;
  *size_p = 2*SqrtN*BLOWUPFACTOR;
}

void
memsetup_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size, int32_t** txmem_p, int32_t* size_p){
  static int32_t txmem[4*SqrtN+2*BLOWUPFACTOR*SqrtN];
  int32_t* E_data_prime = &txmem[2*SqrtN*BLOWUPFACTOR+SqrtN];
  int32_t* E_perm_prime = &txmem[2*SqrtN*BLOWUPFACTOR+2*SqrtN];
  for (int i=0;i<2*BLOWUPFACTOR*SqrtN+4*SqrtN;i++)
    txmem[i] = -1;
  for (int i=0;i<SqrtN;i++)
    txmem[i*(2*BLOWUPFACTOR+1)]=0; 
  for (int i=0;i<SqrtN;i++)
    E_data_prime[i] = M_data[SqrtN*M_data_init+i];
  for (int i=0;i<SqrtN;i++) {
    int cur  = M_perm[SqrtN*M_perm_init+i];
    int off = 4*(2*BLOWUPFACTOR+1)*(cur/SqrtN);
    E_perm_prime[i*2+1] = cur;
    E_perm_prime[i*2] = off;
  }
  // E_perm_prime[i] = i;//SqrtN-1-i;
  //  copy_E_M(M_data, M_perm, E_data, E_perm);
  * txmem_p = txmem;
  * size_p = 4*SqrtN + 2*SqrtN*BLOWUPFACTOR;
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


void app_cleanup_bsort(int32_t* data, int32_t data_init, int32_t data_size){
  int i, j;
  for (i=0;i<data_size-1;i++)
    for (j=0;j<data_size-i-1;j++)
      cas_plain(&data[j],&data[j+1],1);
}

void apptx_merge(int32_t* dst, int32_t*src1,int32_t* src2,int stride) {
  g_starts++;
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
      :"=r"(gContext[0]),"=r"(gContext[1]),"=r"(gContext[2]),"=r"(gContext[3]),"=r"(gContext[4]),
    "=r"(gContext[5]),"=r"(gContext[6]),"=r"(gContext[7]),"=r"(gContext[8]),"=r"(gContext[9]),
    "=r"(gContext[10]),"=r"(gContext[11]),"=r"(gContext[12])
      :"r"(gContext[0]),"r"(gContext[1]),"r"(gContext[2]),"r"(gContext[3]),"r"(gContext[4]),
      "r"(gContext[5]),"r"(gContext[6]),"r"(gContext[7]),"r"(gContext[8]),"r"(gContext[9]),
      "r"(gContext[10]),"r"(gContext[11]),"r"(gContext[12])
           :);
  //memsetup
  for(int p=0;p<stride;p++)
    g_tx_mem[p+2*stride] = src1[p];
  for(int q=0;q<stride;q++)
    g_tx_mem[q+3*stride] = src2[q];
//  EPrintf("before\n");
//  for (int i=0;i<4*stride;i++)
//  EPrintf("%d,",g_tx_mem[i]);
//  EPrintf("\n");
  txbegin(g_tx_mem, 2*stride, stride);
  app_merge();
  /* int i = 0;
     int j= 0;
     int k= 0;
     while (i<stride && j<stride) {
     if (tx_mem[2*stride+i] < tx_mem[3*stride+j]) {tx_mem[k]=tx_mem[2*stride+i];i++;}
     else {tx_mem[k]=tx_mem[3*stride+j];j++;}
     k++;
     }
     while (i<stride) {tx_mem[k]=tx_mem[2*stride+i];i++;k++;}    
     while (j<stride) {tx_mem[k]=tx_mem[3*stride+j];j++;k++;}    
   */
  txend();
//  EPrintf("after\n");
//  for (int i=0;i<4*stride;i++)
//  EPrintf("%d,",g_tx_mem[i]);
//  EPrintf("\n");
  for(int p=0;p<2*stride;p++) {
    dst[p] = g_tx_mem[p];
  }
}

int32_t*  apptxs_cleanup_msort(int32_t* data, int32_t data_init, int32_t data_size){
  int32_t* txmem;
  int32_t txmem_size;
  memsetup_cleanup_msort(data, data_init, data_size, &txmem, &txmem_size);
  int32_t* tmp_data = new int[data_size];
  int32_t* swap;
  for(int stride=2;stride<data_size;stride*=2) {
    for(int j=0; j<data_size; j+=2*stride)
      apptx_merge(tmp_data+j,txmem+j,txmem+j+stride,stride);
    swap = txmem;
    txmem = tmp_data;
    tmp_data = txmem;
  }
  return txmem;
}

void apptx_distribute(int32_t* M_data, int32_t M_data_init, int32_t  M_data_size, int32_t* M_perm, int32_t M_perm_init, int32_t* M_output, int32_t M_output_init, int32_t M_output_size){
  int32_t* txmem;
  int32_t txmem_size;
  g_starts++;
  memsetup_distribute(M_data, M_data_init, M_data_size, M_perm, M_perm_init, M_output, M_output_init, M_output_size, &txmem, &txmem_size);
  //TODO-TODAY pack a function
  //TODO-TODAY relocate recover code to abort handler
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
      :"=r"(gContext[0]),"=r"(gContext[1]),"=r"(gContext[2]),"=r"(gContext[3]),"=r"(gContext[4]),
    "=r"(gContext[5]),"=r"(gContext[6]),"=r"(gContext[7]),"=r"(gContext[8]),"=r"(gContext[9]),
    "=r"(gContext[10]),"=r"(gContext[11]),"=r"(gContext[12])
      :"r"(gContext[0]),"r"(gContext[1]),"r"(gContext[2]),"r"(gContext[3]),"r"(gContext[4]),
      "r"(gContext[5]),"r"(gContext[6]),"r"(gContext[7]),"r"(gContext[8]),"r"(gContext[9]),
      "r"(gContext[10]),"r"(gContext[11]),"r"(gContext[12])
           :);
  txbegin(txmem, 2*BLOWUPFACTOR*SqrtN+SqrtN, M_data_size);
  //    applogic_distribute(txmem, txmem_size);
  app_distribute2();
  txend();
  for(int i=0;i<SqrtN;i++)
    for(int j=0;j<2*BLOWUPFACTOR;j++) {
      M_output[M_output_init*SqrtN*2*BLOWUPFACTOR+i*2*BLOWUPFACTOR+j] = txmem[i*(2*BLOWUPFACTOR+1)+j+1]; 
    }
}

void testMergeSort() {
  int32_t data[8] = {8,1,6,5,7,2,3,4};
  int32_t* data1 = apptxs_cleanup_msort(data,0,8);
  for(int i=0;i<8;i++)
    EPrintf("data1[%d]=%d\n",i,data1[i]);
}

//return 1 if there is a overflow
int checkOFlow(int32_t* aPerm) {
  int count[SqrtN];
  int bucket;
  int max=0;
  for(int i=0;i<SqrtN;i++) {
    memset(count,0,sizeof(int32_t)*SqrtN);
    for(int j=0;j<SqrtN;j++) {
      bucket = aPerm[i*SqrtN+j]/SqrtN;
      if (count[bucket]==BLOWUPFACTOR) return 1;
      count[bucket]++;
    }
  }
  return 0;
}

void swap(int *a,int *b)
{       
  int temp=*a;
  *a=*b;
  *b=temp;
}

void genRandom(int* permutation)
{    
    
  unsigned int seed;
  for (int i=0;i<N;i++) permutation[i]=i;
  for(int i=0;i<=N-2;i++){
    sgx_read_rand((unsigned char*)&seed,4);
    int j = seed%(N-i);
    swap(&permutation[i],&permutation[i+j]);
  }
}

int verify(int32_t* data, int32_t* perm, int32_t* output)
{
  for (int i=0;i<N;i++) 
      if (output[perm[i]]!=data[i]) return 1;
  return 0;
}

/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size)
{
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;
  //TODO-TODAY MOVE: txbegin
  if (4*(2*SqrtN+SqrtN*BLOWUPFACTOR) > c_size) {
    return 0;
  }
    for (int i=0;i<N;i++) EPrintf("%d,",M_perm[i]);
    EPrintf("\n");
  int M_random[N];
  int M_rr[N];
  int M_dr[N];
  //while(1) {
    genRandom(M_random);

    if(checkOFlow(M_random)) { 
      EPrintf("random overflow\n");
     //  continue;
    }
    for (int i = 0; i < SqrtN; i++){
      apptx_distribute(M_perm,i,SqrtN,M_random,i,g_scratch,i,SqrtN*BLOWUPFACTOR);
    }
    /* unit test */
    //testMergeSort();  
    int pos = 0;
    for (int j = 0; j < SqrtN; j++){
      int32_t* ret = apptxs_cleanup_msort(g_scratch,j,2*SqrtN*BLOWUPFACTOR);
      for (int i=0;i<2*SqrtN*BLOWUPFACTOR;i+=2)
        if (ret[i]!=-1) {M_rr[pos] = ret[i+1]; pos++;}
    }
    int aPoint = verify(M_perm,M_random,M_rr);
    if(!aPoint) EPrintf("correct!\n");
    else EPrintf("Wrong\n");
    /*
    if(checkOFlow(M_rr)) {
      EPrintf("pi_r overflow\n");
      continue;
    }
    EPrintf("run here 3\n");
    for (int i = 0; i < SqrtN; i++){
      apptx_distribute(M_data,i,SqrtN,M_random,i,g_scratch,i,SqrtN*BLOWUPFACTOR);
    }
    pos = 0;
    for (int j = 0; j < SqrtN; j++){
      int32_t* ret = apptxs_cleanup_msort(g_scratch,j,2*SqrtN*BLOWUPFACTOR);
      for (int i=0;i<2*SqrtN*BLOWUPFACTOR;i+=2)
        if (ret[i]!=-1) {M_dr[pos] = ret[i+1]; pos++;}
    }
    for (int i = 0; i < SqrtN; i++){
      apptx_distribute(M_dr,i,SqrtN,M_rr,i,g_scratch,i,SqrtN*BLOWUPFACTOR);
    }
    pos = 0;
    for (int j = 0; j < SqrtN; j++){
      int32_t* ret = apptxs_cleanup_msort(g_scratch,j,2*SqrtN*BLOWUPFACTOR);
      for (int i=0;i<2*SqrtN*BLOWUPFACTOR;i+=2)
        if (ret[i]!=-1) {M_output[pos] = ret[i+1]; pos++;}
    }
    break;
  }*/
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

////////TOREMOVE////////

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


