#ifndef _APP_H_
#define _APP_H_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "sgx_eid.h"   /* sgx_enclave_id_t */
#include "sgx_error.h" /* sgx_status_t */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define TOKEN_FILENAME "enclave.token"
#define ENCLAVE_FILENAME "enclave.signed.so"

extern sgx_enclave_id_t global_eid; /* global enclave id */

#if defined(__cplusplus)
extern "C" {
#endif

void ocall_abort(const char* message);

#if defined(__cplusplus)
}
#endif

#endif /* !_APP_H_ */
