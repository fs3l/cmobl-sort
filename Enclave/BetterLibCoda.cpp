#include "./BetterLibCoda.h"
#include <string.h>
// return 0 if success

int32_t ob_dummy[100000];
int32_t nob_dummy[100000];
uint32_t ob_start_dummy[100];
uint32_t nob_start_dummy[100];
uint32_t ob_pos_dummy[100];

int32_t ob_next_dummy(struct CodaIntf_t* intf, uint32_t handle, int32_t* v)
{
  *v = intf->ob_mem[intf->ob_start[handle] + intf->ob_pos[handle]];
  intf->ob_pos[handle]++;
  return 0;
}
int32_t nob_read_dummy(struct CodaIntf_t* intf, uint32_t handle, uint32_t pos,
                       int32_t* v)
{
  *v = intf->nob_mem[intf->nob_start[handle] + pos];
  return 0;
}
int32_t nob_write_dummy(struct CodaIntf_t* intf, uint32_t handle, uint32_t pos,
                        int32_t v)
{
  intf->nob_mem[intf->nob_start[handle] + pos] = v;
  return 0;
}

uint32_t addOb_dummy(struct CodaIntf_t* intf, int32_t* ob, uint32_t size)
{
  for (int i = 0; i < size; i++) intf->ob_mem[intf->ob_global + i] = ob[i];
  intf->ob_handle++;
  intf->ob_start[intf->ob_handle] = intf->ob_global;
  intf->ob_global += size;
  return intf->ob_handle;
}

uint32_t addNob_dummy(struct CodaIntf_t* intf, int32_t* nob, uint32_t size)
{
  for (int i = 0; i < size; i++) intf->nob_mem[intf->nob_global + i] = nob[i];
  intf->nob_handle++;
  intf->nob_start[intf->nob_handle] = intf->nob_global;
  intf->nob_global += size;
  return intf->nob_handle;
}

int32_t coda_finish_dummy(CodaIntf* intf) { return 0; }
int32_t coda_init_dummy(CodaIntf* intf)
{
  intf->nob_read = nob_read_dummy;
  intf->nob_write = nob_write_dummy;
  intf->ob_next = ob_next_dummy;
  intf->addOb = addOb_dummy;
  intf->addNob = addNob_dummy;
  intf->nob_mem = nob_dummy;
  intf->ob_mem = ob_dummy;
  intf->ob_handle = 0;
  intf->nob_handle = 0;
  intf->ob_global = 0;
  intf->nob_global = 0;
  intf->ob_start = ob_start_dummy;
  intf->nob_start = nob_start_dummy;
  intf->ob_pos = ob_pos_dummy;
  memset(ob_start_dummy, 0, sizeof(uint32_t) * 100);
  memset(ob_start_dummy, 0, sizeof(uint32_t) * 100);
  memset(ob_pos_dummy, 0, sizeof(int32_t) * 100);
  memset(ob_dummy, 0, sizeof(int32_t) * 100000);
  memset(nob_dummy, 0, sizeof(int32_t) * 100000);
  return 0;
}

int txbegin_dummy() { return 0; }
int txend_dummy() { return 0; }
