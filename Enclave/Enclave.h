#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <assert.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define NBITS2(n) ((n & 2) ? 1 : 0)
#define NBITS4(n) ((n & (0xC)) ? (2 + NBITS2(n >> 2)) : (NBITS2(n)))
#define NBITS8(n) ((n & 0xF0) ? (4 + NBITS4(n >> 4)) : (NBITS4(n)))
#define NBITS16(n) ((n & 0xFF00) ? (8 + NBITS8(n >> 8)) : (NBITS8(n)))
#define NBITS32(n) ((n & 0xFFFF0000) ? (16 + NBITS16(n >> 16)) : (NBITS16(n)))
#define NBITS(n) (n == 0 ? 0 : NBITS32(n))

//#define CACHE_SIZE 32*1024
//#define SqrtN (CACHE_SIZE/4)
// TOREMOVE
#define SqrtN 64
#define N SqrtN *SqrtN
#define BLOWUPFACTOR 16
//#define BLOWUPFACTOR NBITS(N) //TODO blowupfactor = lg(N) ?
//#define BLOWUPFACTOR 2*log(N)

int EPrintf(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
