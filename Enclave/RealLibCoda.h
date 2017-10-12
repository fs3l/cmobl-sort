#ifndef _REAL_CODA_H
#define _REAL_CODA_H
#include <string.h>
#include "./BetterLibCoda.h"

typedef uint32_t HANDLE;
typedef int32_t DATA;
typedef uint32_t INDEX;
typedef uint32_t LENGTH;
struct RealCoda
{
  // private
  // 0-15 -  meta data
  // 16-31 - ob data r
  // 32 - 79 - ob data rw
  // 80 - 607 - nob data
  // 1024*1023 + 512 - 1024*1024-1 stack frame
  DATA txmem[1024*1024] __attribute__((aligned(64)));
  INDEX cur_ob;
  INDEX cur_nob;
  INDEX cur_meta;
  HANDLE handle;

  RealCoda()
  {
    cur_ob = 0;
    cur_nob = 0;
    cur_meta = 0;
    handle = -1;
    memset(txmem,0,sizeof(DATA)*1024*1024);
  }
};

HANDLE initialize_ob_iterator(DATA* data, LENGTH len);
HANDLE initialize_nob_array(DATA* data, LENGTH len);

DATA nob_read_at(HANDLE h, INDEX pos);
void nob_write_at(HANDLE h, INDEX pos, DATA d);
DATA ob_read_next(HANDLE h);
void ob_write_next(HANDLE h, DATA d);

extern struct RealCoda theCoda;
extern unsigned long old_rsp;

__attribute__((always_inline)) inline void coda_txbegin()
{
  __asm__(
      "mov %1,%%rsi\n\t"
      "mov %%rsp,%0\n\t"
      "mov %2,%%rsp\n\t"
      : "=r"(old_rsp)
      : "r"(theCoda.txmem), "r"(&theCoda.txmem[1024*1024-1])
      : "rdi");
    __asm__("xbegin coda_abort_handler\n\t");
/*  __asm__(
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
      :);*/
}

__attribute__((always_inline)) inline void coda_txend()
{
  __asm__("xend\n\t"
          "mov %0, %%rsp\n\t"
          :
          :"r"(old_rsp)
          :);
}

#endif
