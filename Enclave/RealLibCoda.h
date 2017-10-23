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
  DATA txmem[1024*1024] __attribute__((aligned(4096)));
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

extern "C" {uint64_t coda_stack_switch();}
extern struct RealCoda theCoda;
extern unsigned long old_rsp;
extern unsigned long old_rbp;
extern uint64_t coda_context[100];



//__attribute__((always_inline)) inline void coda_txbegin()
void coda_txbegin();
/*{
    coda_stack_switch(); 
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
      : "=r"(coda_context[0]), "=r"(coda_context[1]), "=r"(coda_context[2]),
        "=r"(coda_context[3]), "=r"(coda_context[4]), "=r"(coda_context[5]),
        "=r"(coda_context[6]), "=r"(coda_context[7]), "=r"(coda_context[8]),
        "=r"(coda_context[9]), "=r"(coda_context[10]), "=r"(coda_context[11]),
        "=r"(coda_context[12])
      : "r"(&coda_context[0])
      :);

    __asm__ ("mov %0,%%rdi\n\t"
            :
            :"r"(theCoda.txmem)
            :"%rdi");
  __asm__(
      "mov $0, %%eax\n\t"
      "mov %%rdi, %%rcx\n\t"
      "loop_ep_%=:\n\t"
      "cmpl  $7690,%%eax\n\t"
      "jge    endloop_ep_%=\n\t"
      "movl   (%%rcx),%%r11d\n\t"
      "addl   $1, %%eax\n\t"
      "add    $4, %%rcx\n\t"
      "jmp    loop_ep_%=\n\t"
      "endloop_ep_%=:\n\t"
      "mov $0, %%eax\n\t"
      "mov %%rdi, %%rcx\n\t"
      "add $1048064, %%rcx\n\t"
      "loop_ep1_%=:\n\t"
      "cmpl  $512,%%eax\n\t"
      "jge    endloop_ep1_%=\n\t"
      "movl   (%%rcx),%%r11d\n\t"
      "addl   $1, %%eax\n\t"
      "add    $4, %%rcx\n\t"
      "jmp    loop_ep1_%=\n\t"
      "endloop_ep1_%=:\n\t"
     // "xbegin coda_abort_handler\n\t"
      "mov $0, %%eax\n\t"
      "mov %%rdi, %%rcx\n\t"
      "loop_ip_%=:\n\t"
      "cmpl  $7690,%%eax\n\t"
      "jge    endloop_ip_%=\n\t"
      "movl   (%%rcx),%%r11d\n\t"
      "addl   $1, %%eax\n\t"
      "add    $4, %%rcx\n\t"
      "jmp    loop_ip_%=\n\t"
      "endloop_ip_%=:\n\t"
      "mov $0, %%eax\n\t"
      "mov %%rdi, %%rcx\n\t"
      "add $1048064, %%rcx\n\t"
      "loop_ip1_%=:\n\t"
      "cmpl  $512,%%eax\n\t"
      "jge    endloop_ip1_%=\n\t"
      "movl   (%%rcx),%%r11d\n\t"
      "addl   $1, %%eax\n\t"
      "add    $4, %%rcx\n\t"
      "jmp    loop_ip1_%=\n\t"
      "endloop_ip1_%=:\n\t"
      :::);
}*/

__attribute__((always_inline)) inline void coda_txend()
{
  __asm__("xend\n\t");
     //     "mov %%rsp, %0\n\t"
     //     "mov %%rbp, %1\n\t"
    //     // "mov %%rbp,%%rsp\n\t"
         // "pop %%rbp\n\t"
         // "pop %%rsp\n\t"
         // "pop %%rsp\n\t"
        //  "mov %%rbp,%%rsp\n\t"
        //  "pop %%rbp\n\t"
       //   "pop %%rsp\n\t"
       //   "pop %%rsp\n\t"
       //   :"=r"(old_rsp),"=r"(old_rbp)
       //   :
       //   :);
 theCoda.cur_ob = 0;
    theCoda.cur_nob = 0;
    theCoda.cur_meta = 0;
    theCoda.handle = -1;
//    memset(theCoda.txmem,0,sizeof(DATA)*1024*1024);

}

#endif
