#ifndef _LIB_CODA_H
#define _LIB_CODA_H

#include "Enclave_t.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int g_aborts;
extern int g_starts;
extern int g_finishes;
extern int32_t cache_size;

__attribute__((always_inline)) inline void txbegin128d2(int32_t* txmem,
                                                        int32_t nob_size,
                                                        int32_t ob_size)
{
    // if (sizeof(int)*(nob_size+ob_size) > cache_size) {
    //  return;
    // }

    __asm__(
        "movl %0,%%esi\n\t"
        "mov %1,%%rdi\n\t"
        "movl %2,%%edx\n\t"
        :
        : "r"(nob_size), "r"(txmem), "r"(ob_size)
        : "esi", "edi", "edx");
    __asm__(
        "movl %%esi, %%r8d\n\t"
        "mov $0, %%eax\n\t"
        "mov %%rdi, %%rcx\n\t"
        "loop_ep_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_ep_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "movl   %%r11d, (%%rcx)\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_ep_%=\n\t"
        "endloop_ep_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob1_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob1_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob1_%=\n\t"
        "endloop_epob1_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob2_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob2_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob2_%=\n\t"
        "endloop_epob2_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob3_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob3_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob3_%=\n\t"
        "endloop_epob3_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob4_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob4_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob4_%=\n\t"
        "endloop_epob4_%=:\n\t"
        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob5_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob5_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob5_%=\n\t"
        "endloop_epob5_%=:\n\t"
        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob6_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob6_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob6_%=\n\t"
        "endloop_epob6_%=:\n\t"
        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob7_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob7_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob7_%=\n\t"
        "endloop_epob7_%=:\n\t"

        "xbegin asm_abort_handler\n\t"
        //  "mov %%rdi,%%rcx\n\t"
        //  "mov $0, %%eax\n\t"
        //  "loop_ip_%=:\n\t"
        //  "cmpl  %%r8d,%%eax\n\t"
        //  "jge    endloop_ip_%=\n\t"
        //  "movl   (%%rcx),%%r11d\n\t"
        //  "movl   %%r11d, (%%rcx)\n\t"
        //  "addl   $1, %%eax\n\t"
        //   "add   $4, %%rcx\n\t"
        //   "jmp    loop_ip_%=\n\t"
        //  "endloop_ip_%=:\n\t"
        ::
            :);
}

__attribute__((always_inline)) inline void txbegin64d2(int32_t* txmem,
                                                       int32_t nob_size,
                                                       int32_t ob_size)
{
    // if (sizeof(int)*(nob_size+ob_size) > cache_size) {
    //  return;
    // }

    __asm__(
        "movl %0,%%esi\n\t"
        "mov %1,%%rdi\n\t"
        "movl %2,%%edx\n\t"
        :
        : "r"(nob_size), "r"(txmem), "r"(ob_size)
        : "esi", "edi", "edx");
    __asm__(
        "movl %%esi, %%r8d\n\t"
        "mov $0, %%eax\n\t"
        "mov %%rdi, %%rcx\n\t"
        "loop_ep_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_ep_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "movl   %%r11d, (%%rcx)\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_ep_%=\n\t"
        "endloop_ep_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob1_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob1_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob1_%=\n\t"
        "endloop_epob1_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob2_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob2_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob2_%=\n\t"
        "endloop_epob2_%=:\n\t"

        "add $1984, %%rcx\n\t"
        "mov $0, %%eax\n\t"
        "loop_epob3_%=:\n\t"
        "cmpl  $528,%%eax\n\t"  //  #r8d is the LSB of the r8
        "jge    endloop_epob3_%=\n\t"
        "movl   (%%rcx),%%r11d\n\t"
        "addl   $1, %%eax\n\t"
        "add    $4, %%rcx\n\t"
        "jmp    loop_epob3_%=\n\t"
        "endloop_epob3_%=:\n\t"

        "xbegin asm_abort_handler\n\t"
        //  "mov %%rdi,%%rcx\n\t"
        //  "mov $0, %%eax\n\t"
        //  "loop_ip_%=:\n\t"
        //  "cmpl  %%r8d,%%eax\n\t"
        //  "jge    endloop_ip_%=\n\t"
        //  "movl   (%%rcx),%%r11d\n\t"
        //  "movl   %%r11d, (%%rcx)\n\t"
        //  "addl   $1, %%eax\n\t"
        //   "add   $4, %%rcx\n\t"
        //   "jmp    loop_ip_%=\n\t"
        //  "endloop_ip_%=:\n\t"
        ::
            :);
}


__attribute__((always_inline)) inline void txbegin(int32_t* txmem,
                                                   int32_t nob_size,
                                                   int32_t ob_size)
{
    if (sizeof(int) * (nob_size + ob_size) > cache_size) {
        return;
    }

    __asm__(
        "movl %0,%%esi\n\t"
        "mov %1,%%rdi\n\t"
        "movl %2,%%edx\n\t"
        :
        : "r"(nob_size), "r"(txmem), "r"(ob_size)
        : "esi", "edi", "edx");
    __asm__(
        "movl %%esi, %%r8d\n\t"
        "mov $0, %%eax\n\t"
        "mov %%rdi, %%rcx\n\t"
        "loop_ep_%=:\n\t"
        "cmpl  %%r8d,%%eax\n\t"  //  #r8d is the LSB of the r8
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
        "endloop_ip_%=:\n\t" ::
            :);
}

__attribute__((always_inline)) inline void txend()
{
    __asm__("xend");
    g_finishes++;
}

void tx_abort(int code);

#ifdef __cplusplus
}
#endif

#endif
