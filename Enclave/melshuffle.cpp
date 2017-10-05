/*
  Author: Ju Chen
  10/04/2017
*/

#include <stdarg.h>
#include <stdio.h> /* vsnprintf */

#include <setjmp.h>
#include <sgx_cpuid.h>
#include <sgx_trts.h>
#include <string.h>

#include "./Enclave.h"
#include "./Enclave_t.h" /* bar*/
#include "./LibCoda.h"
#include "./melshuffle.h"

int EPrintf(const char* fmt, ...);
uint64_t gContext[100];
int32_t g_scratch[2 * BLOWUPFACTOR * N];
// debug
int32_t g_scratch_1[2 * BLOWUPFACTOR * N];
int32_t g_tx_mem[8192];
int32_t g_tx_dis_mem[12 * 1024] __attribute__((aligned(64)));
;
int32_t g_tx_dis_mem128[24 * 1024] __attribute__((aligned(64)));
;

void (*distribute_method)(int32_t*, int32_t, int32_t, int32_t*, int32_t,
                          int32_t*, int32_t, int32_t);
void copy_E_M(int32_t* M_data, int32_t* M_perm, int32_t* E_data,
              int32_t* E_perm)
{
    memcpy(E_data, M_data, N * sizeof(int32_t));
    memcpy(E_perm, M_perm, N * sizeof(int32_t));
    // TODO
}

extern "C" {
int asm_cache_hit_simulate(int32_t* data, int32_t size1);
int asm_cache_miss_simulate(int32_t* data, int32_t size1);

__attribute__((always_inline)) inline void contextsave()
{
    // TODO-TODAY relocate recover code to abort handler
    __asm__(
        "mov %%rax,%0\n\t"
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
        "mov %13,%%r15\n\t"
        "lea (%%rip),%%r14\n\t"
        : "=r"(gContext[0]), "=r"(gContext[1]), "=r"(gContext[2]),
          "=r"(gContext[3]), "=r"(gContext[4]), "=r"(gContext[5]),
          "=r"(gContext[6]), "=r"(gContext[7]), "=r"(gContext[8]),
          "=r"(gContext[9]), "=r"(gContext[10]), "=r"(gContext[11]),
          "=r"(gContext[12])
        : "r"(&gContext[0])
        :);
}
}

__attribute__((always_inline)) inline void app_merge()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
        "add %%r12,%%r8\n\t"   // r8 is src1
        "mov %%r8,%%r9\n\t"    // r9 = data
        "mov %%rdx,%%r12\n\t"  // r12 = size2
        "sall $2,%%r12d\n\t"   // r12=r12*4
        "add %%r12,%%r9\n\t"   // r9 is now src2
        "mov %%rdi,%%r12\n\t"  // r12 is now txmem
        "mov $0,%%eax\n\t"     // eax = i = 0
        "mov $0,%%ebx\n\t"     // ebx = j = 0
        "loop_merge_%=:\n\t"
        "cmpl %%edx,%%eax\n\t"
        "jge endloop_merge_%=\n\t"
        "cmpl %%edx,%%ebx\n\t"
        "jge endloop_merge_%=\n\t"
        "movl (%%r8),%%r10d\n\t"    // src1[i] -> r10
        "movl (%%r9),%%r11d\n\t"    // src2[j] -> r11
        "movl %%r10d,(%%r12)\n\t"   // assign value
        "movl 4(%%r8),%%r13d\n\t"   // src1[i+1] -> r13
        "movl %%r13d,4(%%r12)\n\t"  // assign value
        "addl $2,%%eax\n\t"         // i++
        "add $8,%%r8\n\t"           // i++
        "cmpl %%r10d,%%r11d\n\t"
        "jge another_path_%=\n\t"
        "movl %%r11d,(%%r12)\n\t"   // assign value
        "movl 4(%%r9),%%r13d\n\t"   // src1[i+1] -> r13
        "movl %%r13d,4(%%r12)\n\t"  // assign value
        "addl $2,%%ebx\n\t"         // j++
        "add $8,%%r9\n\t"           // j++
        "subl $2,%%eax\n\t"
        "sub $8,%%r8\n\t"
        "another_path_%=:\n\t"
        "add $8,%%r12\n\t"  // k++
        "jmp loop_merge_%=\n\t"
        "endloop_merge_%=:\n\t"
        "loop_append1_%=:\n\t"
        "cmpl %%edx,%%eax\n\t"
        "jge endloop_append1_%=\n\t"
        "movl (%%r8),%%r10d\n\t"    // src1[i] -> r10
        "movl %%r10d,(%%r12)\n\t"   // assign value
        "movl 4(%%r8),%%r10d\n\t"   // src1[i] -> r10
        "movl %%r10d,4(%%r12)\n\t"  // assign value
        "addl $2,%%eax\n\t"         // i++
        "add $8,%%r8\n\t"           // i++
        "add $8,%%r12\n\t"          // k++
        "jmp loop_append1_%=\n\t"
        "endloop_append1_%=:\n\t"
        "loop_append2_%=:\n\t"
        "cmpl %%edx,%%ebx\n\t"
        "jge endloop_append2_%=\n\t"
        "movl (%%r9),%%r11d\n\t"    // src2[j] -> r11
        "movl %%r11d,(%%r12)\n\t"   // assign value
        "movl 4(%%r9),%%r11d\n\t"   // src2[j] -> r11
        "movl %%r11d,4(%%r12)\n\t"  // assign value
        "addl $2,%%ebx\n\t"         // j++
        "add $8,%%r9\n\t"           // j++
        "add $8,%%r12\n\t"          // k++
        "jmp loop_append2_%=\n\t"
        "endloop_append2_%=:\n\t" ::
            :);
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

__attribute__((always_inline)) inline void app_distribute5()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
        "add $2112,%%r8\n\t"   // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
        "mov %%rdx,%%r12\n\t"  // r12 = size2
        "sall $2,%%r12d\n\t"   // r12=r12*4
        "add $64,%%r9\n\t"     // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis_%=:\n\t"
        "cmpl $16,%%eax\n\t"
        "jge endloop_dis_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis_%=\n\t"
        "endloop_dis_%=:\n\t"

        "add $3968,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis2_%=:\n\t"
        "cmpl $16,%%eax\n\t"
        "jge endloop_dis2_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis2_%=\n\t"
        "endloop_dis2_%=:\n\t"

        "add $3968,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis3_%=:\n\t"
        "cmpl $16,%%eax\n\t"
        "jge endloop_dis3_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis3_%=\n\t"
        "endloop_dis3_%=:\n\t"

        "add $4032,%%r8\n\t"
        "add $3968,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis4_%=:\n\t"
        "cmpl $16,%%eax\n\t"
        "jge endloop_dis4_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis4_%=\n\t"
        "endloop_dis4_%=:\n\t" ::
            :);
}

__attribute__((always_inline)) inline void app_distribute4()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
        "add %%r12,%%r8\n\t"   // r8 is data
        "add $4096, %%r8\n\t"  // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
                               // "mov %%rdx,%%r12\n\t"  //r12 = size2
                               //  "sall $2,%%r12d\n\t"  //r12=r12*4
        "add $16384,%%r9\n\t"  // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis1_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis1_%=\n\t"
        "movl (%%r9),%%r11d\n\t"  // offset[i] -> r11
        // "movl (%%r8),%%r11d\n\t"  // r11 is data
        "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $8,%%r9\n\t"          // increment
        "jmp loop_dis1_%=\n\t"
        "endloop_dis1_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis2_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis2_%=\n\t"
        "movl (%%r9),%%r11d\n\t"  // offset[i] -> r11
        // "movl (%%r8),%%r11d\n\t"  // r11 is data
        "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $8,%%r9\n\t"          // increment
        "jmp loop_dis2_%=\n\t"
        "endloop_dis2_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis3_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis3_%=\n\t"
        "movl (%%r9),%%r11d\n\t"  // offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $8,%%r9\n\t"          // increment
        "jmp loop_dis3_%=\n\t"
        "endloop_dis3_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis4_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis4_%=\n\t"
        "movl (%%r9),%%r11d\n\t"  // offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $8,%%r9\n\t"          // increment
        "jmp loop_dis4_%=\n\t"
        "endloop_dis4_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis5_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis5_%=\n\t"
        "movl (%%r9),%%r11d\n\t"  // offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $8,%%r9\n\t"          // increment
        "jmp loop_dis5_%=\n\t"
        "endloop_dis5_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis6_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis6_%=\n\t"
        //"movl (%%r9),%%r11d\n\t"  //offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        // "movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"  // increment
        "add $4,%%r8\n\t"    // increment
        "add $8,%%r9\n\t"    // increment
        "jmp loop_dis6_%=\n\t"
        "endloop_dis6_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis7_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis7_%=\n\t"
        // "movl (%%r9),%%r11d\n\t"  //offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        //"movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"  // increment
        "add $4,%%r8\n\t"    // increment
        "add $8,%%r9\n\t"    // increment
        "jmp loop_dis7_%=\n\t"
        "endloop_dis7_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis8_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis8_%=\n\t"
        // "movl (%%r9),%%r11d\n\t"  //offset[i] -> r11
        //  "movl (%%r8),%%r11d\n\t"  // r11 is data
        //"movl 4(%%r9),%%r11d\n\t"  // r11 is perm
        "addl $1,%%eax\n\t"  // increment
        "add $4,%%r8\n\t"    // increment
        "add $8,%%r9\n\t"    // increment
        "jmp loop_dis8_%=\n\t"
        "endloop_dis8_%=:\n\t" ::
            :);
}

__attribute__((always_inline)) inline void app_distribute128d2()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
                               // "add %%r12,%%r8\n\t"   //r8 is data
        "add $2112, %%r8\n\t"  // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
                               // "mov %%rdx,%%r12\n\t"  //r12 = size2
                               //  "sall $2,%%r12d\n\t"  //r12=r12*4
        "add $32768,%%r9\n\t"  // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis1_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis1_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis1_%=\n\t"
        "endloop_dis1_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis2_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis2_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis2_%=\n\t"
        "endloop_dis2_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis3_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis3_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis3_%=\n\t"
        "endloop_dis3_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis4_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis4_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis4_%=\n\t"
        "endloop_dis4_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis5_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis5_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis5_%=\n\t"
        "endloop_dis5_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis6_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis6_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis6_%=\n\t"
        "endloop_dis6_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis7_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis7_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis7_%=\n\t"
        "endloop_dis7_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis8_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis8_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis8_%=\n\t"
        "endloop_dis8_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis9_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis9_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis9_%=\n\t"
        "endloop_dis9_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis10_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis10_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis10_%=\n\t"
        "endloop_dis10_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis11_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis11_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis11_%=\n\t"
        "endloop_dis11_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis12_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis12_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis12_%=\n\t"
        "endloop_dis12_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis13_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis13_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis13_%=\n\t"
        "endloop_dis13_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis14_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis14_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis14_%=\n\t"
        "endloop_dis14_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis15_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis15_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis15_%=\n\t"
        "endloop_dis15_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis16_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis16_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis16_%=\n\t"
        "endloop_dis16_%=:\n\t" ::
            :);
}

__attribute__((always_inline)) inline void app_distribute64d2()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
                               // "add %%r12,%%r8\n\t"   //r8 is data
        "add $2112, %%r8\n\t"  // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
                               // "mov %%rdx,%%r12\n\t"  //r12 = size2
                               //  "sall $2,%%r12d\n\t"  //r12=r12*4
        "add $16384,%%r9\n\t"  // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis1_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis1_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis1_%=\n\t"
        "endloop_dis1_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis2_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis2_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis2_%=\n\t"
        "endloop_dis2_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis3_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis3_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis3_%=\n\t"
        "endloop_dis3_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis4_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis4_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis4_%=\n\t"
        "endloop_dis4_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis5_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis5_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis5_%=\n\t"
        "endloop_dis5_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis6_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis6_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis6_%=\n\t"
        "endloop_dis6_%=:\n\t"
        "add $4032,%%r9\n\t"
        "add $4032,%%r8\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis7_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis7_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis7_%=\n\t"
        "endloop_dis7_%=:\n\t"
        "add $4032,%%r9\n\t"
        "mov $0,%%eax\n\t"  // eax = 0
        "loop_dis8_%=:\n\t"
        "cmpl $8,%%eax\n\t"
        "jge endloop_dis8_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis8_%=\n\t"
        "endloop_dis8_%=:\n\t" ::
            :);
}

__attribute__((always_inline)) inline void app_distribute2()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
        "add %%r12,%%r8\n\t"   // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
        "mov %%rdx,%%r12\n\t"  // r12 = size2
        "sall $2,%%r12d\n\t"   // r12=r12*4
        "add %%r12,%%r9\n\t"   // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis_%=:\n\t"
        "cmpl %%edx,%%eax\n\t"
        "jge endloop_dis_%=\n\t"
        "movl (%%r9),%%r11d\n\t"     // offset[i] -> r11
        "mov %%rdi,%%r10\n\t"        // r10 is now txmem
        "add %%r11,%%r10\n\t"        // r10 is now pointer address
        "mov (%%r10),%%r11d\n\t"     // pos -> r11
        "mov %%r10,%%r12\n\t"        // r12  is now pointer adddress
        "add %%r11,%%r10\n\t"        // r10 is now target data address
        "add $4,%%r10\n\t"           // r10 is now target data address
        "movl (%%r8),%%r11d\n\t"     // r11 is data
        "movl %%r11d, 4(%%r10)\n\t"  // assign data
        "movl 4(%%r9),%%r11d\n\t"    // r11 is perm
        "movl %%r11d, (%%r10)\n\t"   // assign perm
        "movl (%%r12),%%r11d\n\t"    // r11d is pointer address
        "addl $8,%%r11d\n\t"         // increment
        "movl %%r11d,(%%r12)\n\t"    // write offset back
        "addl $1,%%eax\n\t"          // increment
        "add $4,%%r8\n\t"            // increment
        "add $8,%%r9\n\t"            // increment
        "jmp loop_dis_%=\n\t"
        "endloop_dis_%=:\n\t" ::
            :);
}

void copy_M_E(int32_t* E_output, int32_t* M_output)
{
    memcpy(M_output, E_output, N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N * BLOWUPFACTOR];
int32_t msortMem[2 * SqrtN * BLOWUPFACTOR];
int32_t bsortMem[2 * SqrtN * BLOWUPFACTOR];
int32_t tmp_data[2 * SqrtN * BLOWUPFACTOR];
void matrix_prepare(int32_t* data, int32_t data_init, int32_t data_size,
                    int32_t** txmem_p, int32_t* size_p)
{
    for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i++)
        msortMem[i] =
            data[i / (2 * BLOWUPFACTOR) * 2 * BLOWUPFACTOR * SqrtN +
                 i % (2 * BLOWUPFACTOR) + data_init * 2 * BLOWUPFACTOR];
    *txmem_p = msortMem;
    *size_p = 2 * SqrtN * BLOWUPFACTOR;
}
void matrix_prepare_bsort(int32_t* data, int32_t data_init, int32_t data_size,
                          int32_t** txmem_p, int32_t* size_p)
{
    for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i++)
        bsortMem[i] =
            data[i / (2 * BLOWUPFACTOR) * 2 * BLOWUPFACTOR * SqrtN +
                 i % (2 * BLOWUPFACTOR) + data_init * 2 * BLOWUPFACTOR];
    *txmem_p = bsortMem;
    *size_p = 2 * SqrtN * BLOWUPFACTOR;
}

void memsetup_distribute128d2(int32_t* M_data, int32_t M_data_init,
                              int32_t M_data_size, int32_t* M_perm,
                              int32_t M_perm_init, int32_t* M_output,
                              int32_t M_output_init, int32_t M_output_size,
                              int32_t** txmem_p, int32_t* size_p)
{
    int start = 0;
    int pos_data = 0;
    int pos_perm = 0;
    memset(g_tx_dis_mem128, 0, sizeof(int32_t) * 24 * 1024);
    for (int i = 0; i < SqrtN / 16; i++) {
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem128[start] = 0;
            for (int k = 1; k < 2 * BLOWUPFACTOR + 1; k++)
                g_tx_dis_mem128[start + j * (2 * BLOWUPFACTOR + 1) + k] = -1;
        }
        start += 1024;
    }
    start = (2 * BLOWUPFACTOR + 1) * 16;
    for (int i = 0; i < SqrtN / 16; i++) {
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem128[start + j] =
                M_data[SqrtN * M_data_init + pos_data++];
        }
        start += 1024;
    }
    for (int i = 0; i < SqrtN / 8; i++) {
        for (int j = 0; j < 8; j++) {
            int cur = M_perm[SqrtN * M_perm_init + pos_perm++];
            int off = 4 * (2 * BLOWUPFACTOR + 1) * ((cur / SqrtN) % 16) +
                      ((cur / SqrtN) / 16) * 4096;
            g_tx_dis_mem128[start + j * 2] = off;
            g_tx_dis_mem128[start + j * 2 + 1] = cur;
        }
        start += 1024;
    }
    *txmem_p = g_tx_dis_mem128;
    *size_p = 12 * 1024;
}

void memsetup_distribute64d2(int32_t* M_data, int32_t M_data_init,
                             int32_t M_data_size, int32_t* M_perm,
                             int32_t M_perm_init, int32_t* M_output,
                             int32_t M_output_init, int32_t M_output_size,
                             int32_t** txmem_p, int32_t* size_p)
{
    int start = 0;
    int pos_data = 0;
    int pos_perm = 0;
    memset(g_tx_dis_mem, 0, sizeof(int32_t) * 12 * 1024);
    for (int i = 0; i < SqrtN / 16; i++) {
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem[start] = 0;
            for (int k = 1; k < 2 * BLOWUPFACTOR + 1; k++)
                g_tx_dis_mem[start + j * (2 * BLOWUPFACTOR + 1) + k] = -1;
        }
        start += 1024;
    }
    start = (2 * BLOWUPFACTOR + 1) * 16;
    for (int i = 0; i < SqrtN / 16; i++) {
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem[start + j] = M_data[SqrtN * M_data_init + pos_data++];
        }
        start += 1024;
    }
    for (int i = 0; i < SqrtN / 8; i++) {
        for (int j = 0; j < 8; j++) {
            int cur = M_perm[SqrtN * M_perm_init + pos_perm++];
            int off = 4 * (2 * BLOWUPFACTOR + 1) * ((cur / SqrtN) % 16) +
                      ((cur / SqrtN) / 16) * 4096;
            g_tx_dis_mem[start + j * 2] = off;
            g_tx_dis_mem[start + j * 2 + 1] = cur;
        }
        start += 1024;
    }
    *txmem_p = g_tx_dis_mem;
    *size_p = 12 * 1024;
}

void memsetup_distribute2(int32_t* M_data, int32_t M_data_init,
                          int32_t M_data_size, int32_t* M_perm,
                          int32_t M_perm_init, int32_t* M_output,
                          int32_t M_output_init, int32_t M_output_size,
                          int32_t** txmem_p, int32_t* size_p)
{
    int start = 0;
    int rack = 0;
    int pos_data = 0;
    int pos_perm = 0;
    for (int i = 0; i < SqrtN / 16; i++) {
        rack = 0;
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem[start + rack] = 0;
            for (int k = 1; k < 2 * BLOWUPFACTOR + 1; k++)
                g_tx_dis_mem[start + rack + k] = -1;
            rack += 2 * BLOWUPFACTOR + 1;
        }
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem[start + rack + j] =
                M_data[SqrtN * M_data_init + pos_data++];
        }
        rack += 16;
        for (int j = 0; j < 16; j++) {
            int cur = M_perm[SqrtN * M_perm_init + pos_perm++];
            int off = 4 * (2 * BLOWUPFACTOR + 1) * ((cur / SqrtN) % 16) +
                      ((cur / SqrtN) / 16) * 4096;
            g_tx_dis_mem[start + rack + j * 2] = off;
            g_tx_dis_mem[start + rack + j * 2 + 1] = cur;
        }
        start += 1024;
    }

    *txmem_p = g_tx_dis_mem;
    *size_p = (SqrtN / 16) * 1024;
}

void memsetup_distribute1(int32_t* M_data, int32_t M_data_init,
                          int32_t M_data_size, int32_t* M_perm,
                          int32_t M_perm_init, int32_t* M_output,
                          int32_t M_output_init, int32_t M_output_size,
                          int32_t** txmem_p, int32_t* size_p)
{
    for (int i = 0; i < 2 * BLOWUPFACTOR * SqrtN + 4 * SqrtN; i++)
        g_tx_dis_mem[i] = -1;
    for (int i = 0; i < SqrtN; i++)
        g_tx_dis_mem[i * (2 * BLOWUPFACTOR + 1)] = 0;
    int start = 2 * BLOWUPFACTOR * SqrtN + SqrtN;
    for (int i = 0; i < SqrtN; i++)
        g_tx_dis_mem[start + i] = M_data[SqrtN * M_data_init + i];
    for (int i = 0; i < SqrtN; i++) {
        int cur = M_perm[SqrtN * M_perm_init + i];
        int off = 4 * (2 * BLOWUPFACTOR + 1) * (cur / SqrtN);
        g_tx_dis_mem[start + SqrtN + i * 2] = off;
        g_tx_dis_mem[start + SqrtN + i * 2 + 1] = cur;
    }

    // for (int i=0;i<16;i++)
    //  g_tx_dis_mem[start+1024+i] = M_data[SqrtN*M_data_init+i];
    start = start + 1024;
    int pos = 0;
    for (int i = 0; i < SqrtN / 16; i++) {
        for (int j = 0; j < 16; j++) {
            g_tx_dis_mem[start + j] = M_data[SqrtN * M_data_init + pos++];
        }
        start += 1024;
    }

    pos = 0;
    for (int i = 0; i < SqrtN / 8; i++) {
        for (int j = 0; j < 8; j++) {
            int cur = M_perm[SqrtN * M_perm_init + pos++];
            int off = 4 * (2 * BLOWUPFACTOR + 1) * (cur / SqrtN);
            g_tx_dis_mem[start + j * 2] = off;
            g_tx_dis_mem[start + j * 2 + 1] = cur;
        }
        start += 1024;
    }
    *txmem_p = g_tx_dis_mem;
    *size_p = 4 * SqrtN + 2 * SqrtN * BLOWUPFACTOR;
}
void memsetup_distribute(int32_t* M_data, int32_t M_data_init,
                         int32_t M_data_size, int32_t* M_perm,
                         int32_t M_perm_init, int32_t* M_output,
                         int32_t M_output_init, int32_t M_output_size,
                         int32_t** txmem_p, int32_t* size_p)
{
    static int32_t txmem[4 * SqrtN + 2 * BLOWUPFACTOR * SqrtN];
    int32_t* E_data_prime = &txmem[2 * SqrtN * BLOWUPFACTOR + SqrtN];
    int32_t* E_perm_prime = &txmem[2 * SqrtN * BLOWUPFACTOR + 2 * SqrtN];
    for (int i = 0; i < 2 * BLOWUPFACTOR * SqrtN + 4 * SqrtN; i++)
        txmem[i] = -1;
    for (int i = 0; i < SqrtN; i++) txmem[i * (2 * BLOWUPFACTOR + 1)] = 0;
    for (int i = 0; i < SqrtN; i++)
        E_data_prime[i] = M_data[SqrtN * M_data_init + i];
    for (int i = 0; i < SqrtN; i++) {
        int cur = M_perm[SqrtN * M_perm_init + i];
        int off = 4 * (2 * BLOWUPFACTOR + 1) * (cur / SqrtN);
        E_perm_prime[i * 2 + 1] = cur;
        E_perm_prime[i * 2] = off;
    }
    // E_perm_prime[i] = i;//SqrtN-1-i;
    //  copy_E_M(M_data, M_perm, E_data, E_perm);
    *txmem_p = txmem;
    *size_p = 4 * SqrtN + 2 * SqrtN * BLOWUPFACTOR;
}

void cas_plain(int* a, int* b, int dir)
{
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

int32_t* apptxs_cleanup_bsort(int32_t* data, int32_t data_init,
                              int32_t data_size)
{
    int32_t* txmem1 = 0;
    int32_t txmem_size1;
    matrix_prepare_bsort(data, data_init, data_size, &txmem1, &txmem_size1);
    for (int i = 0; i < txmem_size1 - 2; i += 2)
        for (int j = 0; j < txmem_size1 - i - 2; j += 2) {
            if (txmem1[j] > txmem1[j + 2]) {
                int tmp = txmem1[j + 2];
                txmem1[j + 2] = txmem1[j];
                txmem1[j] = tmp;
                tmp = txmem1[j + 3];
                txmem1[j + 3] = txmem1[j + 1];
                txmem1[j + 1] = tmp;
            }
        }
    return txmem1;
}

void c_merge(int32_t* dst, int32_t* src1, int32_t* src2, int stride)
{
    // memsetup
    int i = 0;
    int j = 0;
    int k = 0;
    while (i < stride && j < stride) {
        if (src1[i] < src2[j]) {
            dst[k] = src1[i];
            dst[k + 1] = src1[i + 1];
            i += 2;
        } else {
            dst[k] = src2[j];
            dst[k + 1] = src2[j + 1];
            j += 2;
        }
        k += 2;
    }
    while (i < stride) {
        dst[k] = src1[i];
        dst[k + 1] = src1[i + 1];
        i += 2;
        k += 2;
    }
    while (j < stride) {
        dst[k] = src2[j];
        dst[k + 1] = src2[j + 1];
        j += 2;
        k += 2;
    }
}

void apptx_merge(int32_t* dst, int32_t* src1, int32_t* src2, int stride)
{
    g_starts++;
    int32_t otmem[4 * stride] __attribute__((aligned(64)));
    // int32_t otmem[4*stride];
    memset(otmem, 0, 4 * stride * sizeof(int32_t));
    contextsave();
    // memsetup
    for (int p = 0; p < stride; p++) otmem[p + 2 * stride] = src1[p];
    for (int q = 0; q < stride; q++) otmem[q + 3 * stride] = src2[q];
    txbegin(otmem, 2 * stride, stride);
    app_merge();
    /* int i = 0;
       int j= 0;
       int k= 0;
       while (i<stride && j<stride) {
       if (tx_mem[2*stride+i] < tx_mem[3*stride+j])
       {tx_mem[k]=tx_mem[2*stride+i];i++;}
       else {tx_mem[k]=tx_mem[3*stride+j];j++;}
       k++;
       }
       while (i<stride) {tx_mem[k]=tx_mem[2*stride+i];i++;k++;}
       while (j<stride) {tx_mem[k]=tx_mem[3*stride+j];j++;k++;}
     */
    txend();
    for (int p = 0; p < 2 * stride; p++) {
        dst[p] = otmem[p];
    }
}

int32_t* apptxs_cleanup_msort(int32_t* data, int32_t data_init,
                              int32_t data_size)
{
    int32_t* txmem2 = 0;
    int32_t txmem_size;
    matrix_prepare(data, data_init, data_size, &txmem2, &txmem_size);
    int32_t* input;
    int32_t* output;
    input = txmem2;
    output = tmp_data;
    int32_t* swap;
    for (int stride = 2; stride < data_size; stride *= 2) {
        for (int j = 0; j < data_size; j += 2 * stride) {
            apptx_merge(output + j, input + j, input + j + stride, stride);
        }
        // c_merge(output+j,input+j,input+j+stride,stride);}
        swap = input;
        input = output;
        output = swap;
    }
    return input;
}

void apptx_distribute128d2(int32_t* M_data, int32_t M_data_init,
                           int32_t M_data_size, int32_t* M_perm,
                           int32_t M_perm_init, int32_t* M_output,
                           int32_t M_output_init, int32_t M_output_size)
{
    int32_t* txmem;
    int32_t* txmem1;
    int32_t txmem_size;
    g_starts++;
    memsetup_distribute128d2(M_data, M_data_init, M_data_size, M_perm,
                             M_perm_init, M_output, M_output_init,
                             M_output_size, &txmem1, &txmem_size);
    // memsetup_distribute(M_data, M_data_init, M_data_size, M_perm,
    // M_perm_init, M_output, M_output_init, M_output_size, &txmem,
    // &txmem_size);

    /*for (int i=0;i<2112;i++) {
      int index = (i/33)/16*1024 + ((i/33)%16)*33 + i%33;
      if (txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }
    for (int i=2112;i<2176;i++) {
      int index = (i-2112)/16*1024 + (i-2112)%16 + 528;
      if (txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }
    for (int i=2176;i<2304;i++) {
      int index = (i-2176)/16*1024 + 4*1024 + (i-2176)%16 + 528;
      if (i%2==1 && txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }*/
    contextsave();
    txbegin128d2(txmem1, 2 * BLOWUPFACTOR * SqrtN + SqrtN, M_data_size);
    app_distribute128d2();
    txend();
    // txbegin(txmem, 2*BLOWUPFACTOR*SqrtN+SqrtN, M_data_size);
    // app_distribute2();
    // txend();
    //  for(int i=0;i<2*BLOWUPFACTOR*SqrtN+SqrtN;i++) {
    //       EPrintf("txmem[%d]=%d\n",i,txmem[i]);
    //}
    static int count = 0;
    count++;
    // for(int i=0;i<1024*4;i++) {
    //     EPrintf("txmem1[%d]=%d\n",i,txmem1[i]);
    //}
    for (int i = 0; i < SqrtN; i++)
        for (int j = 0; j < 2 * BLOWUPFACTOR; j++) {
            int index =
                (i / 16) * 1024 + (i % 16) * (2 * BLOWUPFACTOR + 1) + j + 1;
            int index1 = i * (2 * BLOWUPFACTOR + 1) + j + 1;
            // if (txmem[i*(2*BLOWUPFACTOR+1)+j+1]!=txmem1[index])
            //    EPrintf("app_distribute 5 is wrong and i=%d,j=%d,
            //    txmem[%d]=%d, and
            //    txmem1[%d]=%d\n",i,j,index1,txmem[index1],index,txmem1[index]);
            M_output[M_output_init * SqrtN * 2 * BLOWUPFACTOR +
                     i * 2 * BLOWUPFACTOR + j] = txmem1[index];
            // M_output[M_output_init*SqrtN*2*BLOWUPFACTOR+i*2*BLOWUPFACTOR+j] =
            // txmem[i*(2*BLOWUPFACTOR+1)+j+1];
        }
}

void apptx_distribute64d2(int32_t* M_data, int32_t M_data_init,
                          int32_t M_data_size, int32_t* M_perm,
                          int32_t M_perm_init, int32_t* M_output,
                          int32_t M_output_init, int32_t M_output_size)
{
    int32_t* txmem;
    int32_t* txmem1;
    int32_t txmem_size;
    g_starts++;
    memsetup_distribute64d2(M_data, M_data_init, M_data_size, M_perm,
                            M_perm_init, M_output, M_output_init, M_output_size,
                            &txmem1, &txmem_size);
    // memsetup_distribute(M_data, M_data_init, M_data_size, M_perm,
    // M_perm_init, M_output, M_output_init, M_output_size, &txmem,
    // &txmem_size);

    /*for (int i=0;i<2112;i++) {
      int index = (i/33)/16*1024 + ((i/33)%16)*33 + i%33;
      if (txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }
    for (int i=2112;i<2176;i++) {
      int index = (i-2112)/16*1024 + (i-2112)%16 + 528;
      if (txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }
    for (int i=2176;i<2304;i++) {
      int index = (i-2176)/16*1024 + 4*1024 + (i-2176)%16 + 528;
      if (i%2==1 && txmem[i]!= txmem1[index])
          EPrintf("memsetup wrong!\n");
    }*/
    contextsave();
    txbegin64d2(txmem1, 2 * BLOWUPFACTOR * SqrtN + SqrtN, M_data_size);
    app_distribute64d2();
    txend();
    // txbegin(txmem, 2*BLOWUPFACTOR*SqrtN+SqrtN, M_data_size);
    // app_distribute2();
    // txend();
    //  for(int i=0;i<2*BLOWUPFACTOR*SqrtN+SqrtN;i++) {
    //       EPrintf("txmem[%d]=%d\n",i,txmem[i]);
    //}
    static int count = 0;
    count++;
    // for(int i=0;i<1024*4;i++) {
    //     EPrintf("txmem1[%d]=%d\n",i,txmem1[i]);
    //}
    for (int i = 0; i < SqrtN; i++)
        for (int j = 0; j < 2 * BLOWUPFACTOR; j++) {
            int index =
                (i / 16) * 1024 + (i % 16) * (2 * BLOWUPFACTOR + 1) + j + 1;
            int index1 = i * (2 * BLOWUPFACTOR + 1) + j + 1;
            // if (txmem[i*(2*BLOWUPFACTOR+1)+j+1]!=txmem1[index])
            //    EPrintf("app_distribute 5 is wrong and i=%d,j=%d,
            //    txmem[%d]=%d, and
            //    txmem1[%d]=%d\n",i,j,index1,txmem[index1],index,txmem1[index]);
            M_output[M_output_init * SqrtN * 2 * BLOWUPFACTOR +
                     i * 2 * BLOWUPFACTOR + j] = txmem1[index];
            // M_output[M_output_init*SqrtN*2*BLOWUPFACTOR+i*2*BLOWUPFACTOR+j] =
            // txmem[i*(2*BLOWUPFACTOR+1)+j+1];
        }
}

void apptx_distribute(int32_t* M_data, int32_t M_data_init, int32_t M_data_size,
                      int32_t* M_perm, int32_t M_perm_init, int32_t* M_output,
                      int32_t M_output_init, int32_t M_output_size)
{
    int32_t* txmem;
    int32_t txmem_size;
    g_starts++;
    memsetup_distribute(M_data, M_data_init, M_data_size, M_perm, M_perm_init,
                        M_output, M_output_init, M_output_size, &txmem,
                        &txmem_size);
    contextsave();
    txbegin(txmem, 2 * BLOWUPFACTOR * SqrtN + SqrtN, M_data_size);
    //    applogic_distribute(txmem, txmem_size);
    app_distribute2();
    txend();
    for (int i = 0; i < SqrtN; i++)
        for (int j = 0; j < 2 * BLOWUPFACTOR; j++) {
            M_output[M_output_init * SqrtN * 2 * BLOWUPFACTOR +
                     i * 2 * BLOWUPFACTOR + j] =
                txmem[i * (2 * BLOWUPFACTOR + 1) + j + 1];
        }
}

void testMergeSort()
{
    int32_t data[8] = {8, 1, 6, 5, 7, 2, 3, 4};
    int32_t* data1 = apptxs_cleanup_msort(data, 0, 8);
    for (int i = 0; i < 8; i++) EPrintf("data1[%d]=%d\n", i, data1[i]);
}

// return 1 if there is a overflow
int checkOFlow(int32_t* aPerm)
{
    int count[SqrtN];
    int bucket;
    int max = 0;
    for (int i = 0; i < SqrtN; i++) {
        memset(count, 0, sizeof(int32_t) * SqrtN);
        for (int j = 0; j < SqrtN; j++) {
            bucket = aPerm[i * SqrtN + j] / SqrtN;
            if (count[bucket] == BLOWUPFACTOR) return 1;
            count[bucket]++;
        }
    }
    return 0;
}

void swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void genRandom(int* permutation)
{
    unsigned int seed;
    for (int i = 0; i < N; i++) permutation[i] = i;
    for (int i = 0; i <= N - 2; i++) {
        sgx_read_rand((unsigned char*)&seed, 4);
        int j = seed % (N - i);
        swap(&permutation[i], &permutation[i + j]);
    }
}

int verify(int32_t* data, int32_t* perm, int32_t* output)
{
    for (int i = 0; i < N; i++)
        if (output[perm[i]] != data[i]) return 1;
    return 0;
}

int compare(const void* a, const void* b) { return (*(int*)a - *(int*)b); }
// mel-shuffle
int melshuffle(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size,
               int input_size)
{
    // input validation and method selection
    if (!(input_size == 2 * 2 || input_size == 4 * 4 || input_size == 8 * 8 ||
          input_size == 16 * 16 || input_size == 32 * 32 ||
          input_size == 64 * 64 || input_size == 128 * 128))
        return 0;

    // method selection
    if (input_size == 128 * 128) {
        distribute_method = apptx_distribute128d2;
    } else if (input_size == 64 * 64) {
        distribute_method = apptx_distribute64d2;
    } else
        distribute_method = apptx_distribute;

    int32_t* M_data = (int32_t*)M_data_ref;
    int32_t* M_perm = (int32_t*)M_perm_ref;
    int32_t* M_output = (int32_t*)M_output_ref;
    cache_size = c_size;
    int M_random[N];
    int M_rr[N];
    int M_dr[N];
    long sec_begin[1], sec_end[1], usec_begin[1], usec_end[1];
    ocall_gettimenow(sec_begin, usec_begin);
    while (1) {
        genRandom(M_random);

        /* shuffle pass 1  PiR = shuffle(Pi,R);*/

        if (checkOFlow(M_random)) {
            EPrintf("random overflow\n");
            continue;
        }
        for (int i = 0; i < SqrtN; i++) {
            distribute_method(M_perm, i, SqrtN, M_random, i, g_scratch, i,
                              SqrtN * BLOWUPFACTOR);
        }
        /* unit test */
        // testMergeSort();

        int pos = 0;
        for (int j = 0; j < SqrtN; j++) {
            int32_t* ret =
                apptxs_cleanup_msort(g_scratch, j, 2 * SqrtN * BLOWUPFACTOR);
            for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR - 1; i += 2)
                if (ret[i] != -1) {
                    M_rr[pos] = ret[i + 1];
                    pos++;
                }
        }
        int aPoint = verify(M_perm, M_random, M_rr);
        if (!aPoint)
            EPrintf("distrbute correct!\n");
        else
            EPrintf("distribute Wrong\n");

        if (checkOFlow(M_rr)) {
            EPrintf("pi_r overflow\n");
            continue;
        }

        // shuffle pass 2  Dr = shuffle(D,R);
        for (int i = 0; i < SqrtN; i++) {
            distribute_method(M_data, i, SqrtN, M_random, i, g_scratch, i,
                              SqrtN * BLOWUPFACTOR);
        }
        pos = 0;
        for (int j = 0; j < SqrtN; j++) {
            int32_t* ret =
                apptxs_cleanup_msort(g_scratch, j, 2 * SqrtN * BLOWUPFACTOR);
            for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i += 2)
                if (ret[i] != -1) {
                    M_dr[pos] = ret[i + 1];
                    pos++;
                }
        }

        // shuffle pass 3   O = shuffle(Dr,PiR)
        for (int i = 0; i < SqrtN; i++) {
            distribute_method(M_dr, i, SqrtN, M_rr, i, g_scratch, i,
                              SqrtN * BLOWUPFACTOR);
        }
        pos = 0;
        for (int j = 0; j < SqrtN; j++) {
            int32_t* ret =
                apptxs_cleanup_msort(g_scratch, j, 2 * SqrtN * BLOWUPFACTOR);
            for (int i = 0; i < 2 * SqrtN * BLOWUPFACTOR; i += 2)
                if (ret[i] != -1) {
                    M_output[pos] = ret[i + 1];
                    pos++;
                }
        }
        break;
    }

    ocall_gettimenow(sec_end, usec_end);
    long t1 =
        (sec_end[0] - sec_begin[0]) * 1000000 + (usec_end[0] - usec_begin[0]);
    qsort(M_output, N, sizeof(int), compare);
    ocall_gettimenow(sec_end, usec_end);
    long t2 =
        (sec_end[0] - sec_begin[0]) * 1000000 + (usec_end[0] - usec_begin[0]);
    EPrintf("t_shuffle=%ld and t_shuffle+t_sort=%ld\n", t1, t2);
    return 0;
}

////////TOREMOVE////////

__attribute__((always_inline)) inline void app_distribute()
{
    __asm__(
        "mov %%rdi,%%r8\n\t"   // r8 = txmem
        "mov %%rsi,%%r12\n\t"  // r12 = size1
        "sall $2,%%r12d\n\t"   // r12=4*size1
        "add %%r12,%%r8\n\t"   // r8 is data
        "mov %%r8,%%r9\n\t"    // r9 = data
        "mov %%rdx,%%r12\n\t"  // r12 = size2
        "sall $2,%%r12d\n\t"   // r12=r12*4
        "add %%r12,%%r9\n\t"   // r9 is now permu
        "mov $0,%%eax\n\t"     // eax = 0
        "loop_dis_%=:\n\t"
        "cmpl %%edx,%%eax\n\t"
        "jge endloop_dis_%=\n\t"
        "movl (%%r8),%%r10d\n\t"   // data[i] -> r10
        "movl (%%r9),%%r11d\n\t"   // permu[i] -> r11
        "sall $2,%%r11d\n\t"       // r11 = r11 *4
        "mov %%rdi,%%rcx\n\t"      // rcx = txmem
        "add  %%r11,%%rcx\n\t"     // rcx is now output[permu[i]]
        "movl %%r10d,(%%rcx)\n\t"  // assign value
        "addl $1,%%eax\n\t"        // increment
        "add $4,%%r8\n\t"          // increment
        "add $4,%%r9\n\t"          // increment
        "jmp loop_dis_%=\n\t"
        "endloop_dis_%=:\n\t" ::
            :);
}

void applogic_distribute(int32_t* txmem, int32_t size)
{
    // int tmp[1000];// = new int[1000];
    // asm_cache_miss_simulate(tmp,1000);
    // int ret = asm_apptx_distribute(txmem,SqrtN*BLOWUPFACTOR,SqrtN);
    return;
}
