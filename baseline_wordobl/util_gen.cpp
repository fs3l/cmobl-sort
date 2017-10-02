#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>

#include "Enclave/Enclave.h"
void generate_file(int n) {
  FILE *fp;
  int32_t* perm = new int32_t[n];
  for(int i=0;i<n;i++)
    perm[i]=i;
  //permutation into file
  fp=fopen("/home/ju/workspace/sgxshuffle/perm.dat","wb");	
  for(int z=0;z<n;z++)
    fwrite(&perm[z],sizeof(int),1,fp);
  fclose(fp);

  int32_t* data = new int32_t[n];
  for(int i=0;i<n;i++)
    data[i]=i;
  fp=fopen("/home/ju/workspace/sgxshuffle/data.dat","wb");	
  for(int z=0;z<n;z++)
    fwrite(&data[z],sizeof(int),1,fp);
  fclose(fp);
}

int main(int argc, char *argv[]){
    int32_t n=(CACHE_SIZE/4) * (CACHE_SIZE/4);
    generate_file(n);
}
