#ifndef _LIB_CODA_H
#define _LIB_CODA_H
#include "Enclave_t.h"
typedef struct CodaIntf_t {
  // public interfaces
  int32_t (*ob_next)(struct CodaIntf_t*, uint32_t handle, int32_t* v);
  int32_t (*nob_read)(struct CodaIntf_t*, uint32_t handle, uint32_t pos,
                      int32_t* v);
  int32_t (*nob_write)(struct CodaIntf_t*, uint32_t handle, uint32_t pos,
                       int32_t v);
  uint32_t (*addOb)(struct CodaIntf_t*, int32_t* ob, uint32_t size);
  uint32_t (*addNob)(struct CodaIntf_t*, int32_t* nob, uint32_t size);
  // private memory don't touch
  int32_t* nob_mem;
  int32_t* ob_mem;
  uint32_t ob_global;
  uint32_t nob_global;
  uint32_t* ob_start;
  uint32_t* nob_start;
  uint32_t* ob_pos;
  uint32_t ob_handle;
  uint32_t nob_handle;
} CodaIntf;

int coda_init_dummy(CodaIntf* intf);
int txbegin_dummy();
int txend_dummy();
int coda_finish_dummy(CodaIntf* intf);
#endif
