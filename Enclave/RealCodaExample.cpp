#include "./BetterLibCoda.h"
#include "./RealLibCoda.h"
#include "./Enclave.h"

int real_coda_shuffle()
{
  int32_t data[4] = {100, 200, 300, 400};
  int32_t perm[4] = {3, 1, 0, 2};
  int32_t output[4];

  HANDLE iterData = initialize_ob_iterator(data,4);
  HANDLE iterPerm = initialize_ob_iterator(perm,4);
  HANDLE arrayOutput = initialize_nob_array(output,4);

  coda_txbegin();
  // app logic start
  for (int i = 0; i < 4; i++) {
    int v1, v2;
    v1= ob_read_next(iterData);
    v2 =ob_read_next(iterPerm);
    nob_write_at(arrayOutput,v2, v1);
  }
  // app logic end
  coda_txend();

  for (int i = 0; i < 4; i++) {
    int v = nob_read_at(arrayOutput,i);
    output[i] = v;
    EPrintf("output[%d]=%d\n", i, output[i]);
  }
  EPrintf("coda shuffle complete\n");
}
