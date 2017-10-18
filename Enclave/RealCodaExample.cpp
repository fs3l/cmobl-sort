#include "./BetterLibCoda.h"
#include "./RealLibCoda.h"
#include "./Enclave.h"
int EPrintf(const char* fmt, ...);
int coda_shuffle(long M_data_ref, long M_perm_ref, long M_output_ref, int c_size,
    int input_size)
{

  int32_t* data = (int32_t*)M_data_ref;
  int32_t* perm = (int32_t*)M_perm_ref;
  int32_t* output = (int32_t*)M_output_ref;
  HANDLE iterData = initialize_ob_iterator(data,N);
  HANDLE iterPerm = initialize_ob_iterator(perm,N);
  HANDLE arrayOutput = initialize_nob_array(output,N);

  long sec_begin[1],sec_end[1],usec_begin[1],usec_end[1];
  ocall_gettimenow(sec_begin,usec_begin);
  coda_txbegin();
  // app logic start
  for (int i = 0; i < N; i++) {
    int v1, v2;
    v1= ob_read_next(iterData);
    v2 =ob_read_next(iterPerm);
    nob_write_at(arrayOutput,v2, v1);
  }
  // app logic end
  coda_txend();
  ocall_gettimenow(sec_end,usec_end);
  EPrintf("time=%ld\n", ((sec_end[0] * 1000000 + usec_end[0]) -
                   (sec_begin[0]* 1000000 + usec_begin[0])));
/*
  for (int i = 0; i < 4; i++) {
    int v = nob_read_at(arrayOutput,i);
    output[i] = v;
    EPrintf("output[%d]=%d\n", i, output[i]);
  }
*/
  EPrintf("coda shuffle complete\n");
}
