#include "./BetterLibCoda.h"
#include "./Enclave.h"
int EPrintf(const char* fmt, ...);

int coda_shuffle()
{
  int32_t data[4] = {100, 200, 300, 400};
  int32_t perm[4] = {3, 1, 0, 2};
  int32_t output[4];

  CodaIntf myIntf;
  if (coda_init_dummy(&myIntf)) return -1;

  uint32_t ob_handle1, ob_handle2, nob_handle;
  ob_handle1 = myIntf.addOb(&myIntf, data, 4);
  ob_handle2 = myIntf.addOb(&myIntf, perm, 4);
  nob_handle = myIntf.addNob(&myIntf, output, 4);

  if (txbegin_dummy()) return -1;

  // app logic start
  for (int i = 0; i < 4; i++) {
    int v1, v2;
    myIntf.ob_next(&myIntf, ob_handle1, &v1);
    myIntf.ob_next(&myIntf, ob_handle2, &v2);
    myIntf.nob_write(&myIntf, nob_handle, v2, v1);
  }
  // app logic end

  if (txend_dummy()) return -1;
  if (coda_finish_dummy(&myIntf)) return -1;

  for (int i = 0; i < 4; i++) {
    int v = 0;
    myIntf.nob_read(&myIntf, nob_handle, i, &v);
    output[i] = v;
    EPrintf("output[%d]=%d\n", i, output[i]);
  }
  EPrintf("coda shuffle complete\n");
}
