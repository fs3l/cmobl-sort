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
  // 16-31 - nob data
  // 32 - 527 - ob data
  // 1024*1023 - 1024*1024-1 stack frame
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

HANDLE declare_ob_iterator(DATA* data, LENGTH len);
HANDLE declare_nob_array(DATA* data, LENGTH len);

DATA nob_read_at(HANDLE h, INDEX pos);
void nob_write_at(HANDLE h, INDEX pos, DATA d);
DATA ob_read_next(HANDLE h);
void ob_write_next(HANDLE h, DATA d);

#endif
