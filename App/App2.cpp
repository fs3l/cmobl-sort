#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
# include <time.h>
# include <sys/time.h>
//#include "sgx_status.h"
#include "App.h"
#include "../Enclave/Enclave.h"
#include "Enclave_u.h"
#include <pthread.h>
#include <sched.h>
#define SIZE_MASK 0xfff
#define WAYS_MASK 0xffc00000
void cpuinfo(int code, int *eax, int *ebx, int *ecx, int *edx) {
  __asm__(
      "mov %4,%%eax\n\t"
      "mov %5,%%ecx\n\t"
      "cpuid\n\t" //  call cpuid instruction
      "mov %%eax,%0\n\t"
      "mov %%ebx,%1\n\t"
      "mov %%ecx,%2\n\t"
      "mov %%edx,%3\n\t"
    //  :"=r"(*ebx)// output equal to "movl  %%eax %1"
      :"=r"(*eax),"=r"(*ebx),"=r"(*ecx),"=r"(*edx)// output equal to "movl  %%eax %1"
      :"r"(code),"r"(*ecx)// input equal to "movl %1, %%eax"
      :"%eax","%ebx","%ecx","%edx"// clobbered register
  );
}

int ecall_foo1(long M_data, long M_perm, long M_output, int c_size)
{
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  int retval;
  ret = ecall_foo(global_eid, &retval, M_data, M_perm, M_output, c_size);

  if (ret != SGX_SUCCESS)
    abort();

  int cpuid[4] = {0x1, 0x0, 0x0, 0x0};
  ret = ecall_sgx_cpuid(global_eid, cpuid, 0x0);
  if (ret != SGX_SUCCESS)
    abort();
  return retval;
}

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
  {
    SGX_ERROR_UNEXPECTED,
    "Unexpected error occurred.",
    NULL
  },
  {
    SGX_ERROR_INVALID_PARAMETER,
    "Invalid parameter.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_MEMORY,
    "Out of memory.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_LOST,
    "Power transition occurred.",
    "Please refer to the sample \"PowerTransition\" for details."
  },
  {
    SGX_ERROR_INVALID_ENCLAVE,
    "Invalid enclave image.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ENCLAVE_ID,
    "Invalid enclave identification.",
    NULL
  },
  {
    SGX_ERROR_INVALID_SIGNATURE,
    "Invalid enclave signature.",
    NULL
  },
  {
    SGX_ERROR_OUT_OF_EPC,
    "Out of EPC memory.",
    NULL
  },
  {
    SGX_ERROR_NO_DEVICE,
    "Invalid SGX device.",
    "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
  },
  {
    SGX_ERROR_MEMORY_MAP_CONFLICT,
    "Memory map conflicted.",
    NULL
  },
  {
    SGX_ERROR_INVALID_METADATA,
    "Invalid enclave metadata.",
    NULL
  },
  {
    SGX_ERROR_DEVICE_BUSY,
    "SGX device was busy.",
    NULL
  },
  {
    SGX_ERROR_INVALID_VERSION,
    "Enclave version was invalid.",
    NULL
  },
  {
    SGX_ERROR_INVALID_ATTRIBUTE,
    "Enclave was not authorized.",
    NULL
  },
  {
    SGX_ERROR_ENCLAVE_FILE_ACCESS,
    "Can't open enclave file.",
    NULL
  },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if(ret == sgx_errlist[idx].err) {
      if(NULL != sgx_errlist[idx].sug)
        printf("Info: %s\n", sgx_errlist[idx].sug);
      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }

  if (idx == ttl)
    printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Step 1: retrive the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void)
{
  char token_path[MAX_PATH] = {'\0'};
  sgx_launch_token_t token = {0};
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  int updated = 0;

  /* Step 1: retrive the launch token saved by last transaction */

  /* __GNUC__ */
  /* try to get the token saved in $HOME */
  const char *home_dir = getpwuid(getuid())->pw_dir;

  if (home_dir != NULL && 
      (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
    /* compose the token path */
    strncpy(token_path, home_dir, strlen(home_dir));
    strncat(token_path, "/", strlen("/"));
    strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
  } else {
    /* if token path is too long or $HOME is NULL */
    strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
  }

  FILE *fp = fopen(token_path, "rb");
  if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
    printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
  }

  if (fp != NULL) {
    /* read the token from saved file */
    size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
    if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
      /* if token is invalid, clear the buffer */
      memset(&token, 0x0, sizeof(sgx_launch_token_t));
      printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
    }
  }

  /* Step 2: call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    if (fp != NULL) fclose(fp);
    return -1;
  }

  /* Step 3: save the launch token if it is updated */
  /* __GNUC__ */
  if (updated == FALSE || fp == NULL) {
    /* if the token is not updated, or file handler is invalid, do not perform saving */
    if (fp != NULL) fclose(fp);
    return 0;
  }

  /* reopen the file with write capablity */
  fp = freopen(token_path, "wb", fp);
  if (fp == NULL) return 0;
  size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
  if (write_num != sizeof(sgx_launch_token_t))
    printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
  fclose(fp);
  return 0;
}

/* OCall functions */
void ocall_bar(const char *str, int ret[1])
{
  /* Proxy/Bridge will check the length and null-terminate 
   * the input string to prevent buffer overflow. 
   */
  printf("%s", str);
  ret[0] = 13;
}

void
util_copy_M_D_file(int32_t* arr, char * filename){
  /*  FILE *file; 
      file=fopen(filename,"rb");
      for(int z=0;z<N;z++)
      fread(&arr[z],sizeof(int32_t),1,file);
      fclose(file);
   */
  //Mel-MD
  FILE *file; 
  file=fopen(filename,"rb");
  for(int z=0;z<N;z++)
    fread(&arr[z],sizeof(int32_t),1,file);
  fclose(file);
}

void
util_copy_D_M_file(int32_t* arr, char * filename){
  /*
     FILE *file; 
     file=fopen(filename,"wb");
     for(int z=0;z<N;z++)
     fwrite(&arr[z],sizeof(int32_t),1,file);
     fclose(file);
   */
  //mel-MD
  FILE *file; 
  file=fopen(filename,"wb");
  fwrite(&arr,sizeof(int32_t),N,file);
  fclose(file);
}

void copy_M_D(int32_t* M_data, int32_t* M_perm){
  util_copy_M_D_file(M_data, "/home/ju/workspace/sgxshuffle/data.dat");
  util_copy_M_D_file(M_perm, "/home/ju/workspace/sgxshuffle/perm.dat");
}

void copy_D_M(int32_t* M_output){
  util_copy_D_M_file(M_output, "/home/ju/workspace/sgxshuffle/output.dat");
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
  /* Initialize the enclave */
  printf("a\n");
  if(initialize_enclave() < 0){printf("Error enclave and exit\n");return -1;}
  printf("b\n");

  /* Utilize edger8r attributes */
  edger8r_function_attributes();

  printf("c\n");
  /* Utilize trusted libraries */
  int retval;

  int32_t* M_data = new int32_t[N];
  int32_t* M_perm = new int32_t[N];
  int32_t* M_output = new int32_t[N*BLOWUPFACTOR];
  for (int i=0;i<N*BLOWUPFACTOR;i++)
    M_output[i]=-1;
  int eax=0,ebx=0,ecx=1,edx=0,cl_size=0,n_ways=0;
  int c_size = 0;
  copy_M_D(M_data, M_perm);
  struct sched_param param;
  param.sched_priority = 0;
  sched_setscheduler(0,SCHED_FIFO,&param);
  cpuinfo(0x04,&eax,&ebx,&ecx,&edx);
  cl_size = ebx & SIZE_MASK;
  n_ways = (ebx & WAYS_MASK)>>22;
  c_size = (cl_size+1)*(n_ways+1)*(ecx+1);
  for (int i=0;i<N;i++) M_data[i] = i;
  M_perm[0] = 1;  
  M_perm[1] = 3;  
  M_perm[2] = 2;  
  M_perm[3] = 0;  
  struct timeval start,end;
  gettimeofday(&start,NULL);
  retval=ecall_foo1((long)M_data, (long)M_perm, (long)M_output, c_size);
  gettimeofday(&end,NULL);
 // printf("eax=%x,ebx=%x,ecx=%x,edx=%x\n",eax,ebx,ecx,edx);
 // printf("cl_size=%d,n_ways=%d,sets=%d\n",cl_size,n_ways,ecx);
  printf("%ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
        - (start.tv_sec * 1000000 + start.tv_usec)));
//    printf("retval: %d\n", retval);
    //TODO this call is buggy, FIXME
    //    copy_D_M(M_output, j);

  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid);
  printf("Info: SampleEnclave successfully returned.\n");
  return 0;
}

