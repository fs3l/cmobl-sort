#include "./RealLibCoda.h"
extern int coda_aborts;
extern "C" {
  void asm_cache_hit_simulate(int* data, int size);
  void asm_cache_miss_simulate(int* data, int size);
}
void exp() {
  int mem[64];
  for (int i=0;i<10000000;i++) {
//  exp_txbegin(mem);
//  asm_cache_hit_simulate(mem,512);
  coda_txend();
  }
  EPrintf("g_aborts=%d\n",coda_aborts);
}
