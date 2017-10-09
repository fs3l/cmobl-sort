#include "./BetterLibCoda.h"
#include "./RealLibCoda.h"
#include "./Enclave.h"
int EPrintf(const char* fmt, ...);

int real_coda_shuffle()
{
  int32_t data[4] = {100, 200, 300, 400};
  int32_t perm[4] = {3, 1, 0, 2};
  int32_t output[4];

  Coda* myCoda = new RealCoda();

  ObIterator* iterData = myCoda->make_ob_iterator(data,4);
  ObIterator* iterPerm = myCoda->make_ob_iterator(perm,4);
  NobArray* arrayOutput = myCoda->make_nob_array(output,4);

//  if (txbegin_dummy()) return -1;

  // app logic start
  for (int i = 0; i < 4; i++) {
    int v1, v2;
    v1= iterData->read_next();
    v2 =iterPerm->read_next();
    arrayOutput->write_at(v2, v1);
  }
  // app logic end

 // if (txend_dummy()) return -1;

  for (int i = 0; i < 4; i++) {
    int v = arrayOutput->read_at(i);
    output[i] = v;
    EPrintf("output[%d]=%d\n", i, output[i]);
  }
  EPrintf("coda shuffle complete\n");
}
