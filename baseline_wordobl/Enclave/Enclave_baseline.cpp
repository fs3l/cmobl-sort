#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <string.h>
#include <sgx_cpuid.h>

#include "sgx_trts.h"
#include "Enclave.h"
#include "Enclave_t.h"  /* bar*/

int baseline();
/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int bar1(const char *fmt, ...)
{
  int ret[1];
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_bar(buf, ret);
  return ret[0];
}

void copy_E_M(int32_t* M_data, int32_t* M_perm, 
    int32_t* E_data,
    int32_t* E_perm){
  memcpy(E_data, M_data, N*sizeof(int32_t));
  memcpy(E_perm, M_perm, N*sizeof(int32_t));

  //TODO
}



void copy_M_E(int32_t* E_output, int32_t* M_output){
  memcpy(M_output,E_output,N * sizeof(int32_t));
}

int32_t E_data[N];
int32_t E_perm[N];
int32_t E_output[N];
/* ecall_foo:
 *   Uses malloc/free to allocate/free trusted memory.
 */
int ecall_foo(long M_data_ref, long M_perm_ref, long M_output_ref)
{
  int32_t* M_data = (int32_t*)M_data_ref;
  int32_t* M_perm = (int32_t*)M_perm_ref;
  int32_t* M_output = (int32_t*)M_output_ref;

  copy_E_M(M_data, M_perm, E_data, E_perm);
  baseline();
  return 0;
}



/* ecall_sgx_cpuid:
 *   Uses sgx_cpuid to get CPU features and types.
 */
void ecall_sgx_cpuid(int cpuinfo[4], int leaf)
{
  sgx_status_t ret = sgx_cpuid(cpuinfo, leaf);
  if (ret != SGX_SUCCESS)
    abort();
}



int32_t matrix[N*N];
int32_t output[N];
void transform(int32_t* p,int32_t* matrix,int n)
{
  int i;
  for(i=0;i<n;i++)
  {
    matrix[p[i]*n+i]=1; 
  }
}

int baseline()
{
  int i,j;

  memset(matrix,0,sizeof(int)*N*N);

  transform(E_perm,matrix,N);


  for(i=0;i<N;i++)
  {
    int x=0;
    for(j=i*N;j<i*N+N;j++)
    { 
      if(matrix[j]==1)
      { x=j-i*N;
        //printf("x=%d\n",x);
      }
    }
    output[i]=E_data[x];
  }

  return 0;
}
