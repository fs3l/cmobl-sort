#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define CACHE_SIZE 32*1024
#define D_N ((CACHE_SIZE/4) * (CACHE_SIZE/4)) 
#define M_N (CACHE_SIZE/4)
#define BLOWUPFACTOR 1 //TODO 
//#define BLOWUPFACTOR 2*log(D_N)

int bar1(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
