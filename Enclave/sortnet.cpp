#include "./Enclave.h"
#include "./Enclave_t.h"
void cas_v2m(int* a,int* b, int* c1,int* c2,int dir) {
	if (dir)
		__asm__(
			"mov	(%0), %%eax\n\t"
			"mov	(%1), %%ebx\n\t"
			"mov	(%2), %%ecx\n\t"
			"mov	(%3), %%edx\n\t"
			"cmp	%%ecx, %%edx\n\t"
			"jg    out1mg\n\t"
			"mov	%%ebx, (%0)\n\t"
			"mov	%%eax, (%1)\n\t"
			"mov	%%edx, (%2)\n\t"
			"mov	%%ecx, (%3)\n\t"
			"out1mg:\n\t"
			:
			:"r" (a), "r" (b), "r" (c1), "r" (c2)
			:"eax","ebx","ecx","edx"
			);
	else
		__asm__(
			"mov	(%0), %%eax\n\t"
			"mov	(%1), %%ebx\n\t"
			"mov	(%2), %%ecx\n\t"
			"mov	(%3), %%edx\n\t"
			"cmp	%%ecx, %%edx\n\t"
			"jl    out1ml\n\t"
			"mov	%%ebx, (%0)\n\t"
			"mov	%%eax, (%1)\n\t"
			"mov	%%edx, (%2)\n\t"
			"mov	%%ecx, (%3)\n\t"
			"out1ml:\n\t"
			:
			:"r" (a), "r" (b), "r" (c1), "r" (c2)
			:"eax","ebx","ecx","edx"
			);
}

static inline void bitonicSort(int n, int start, int* array, int* base) {
  if (n==2)
    cas_v2m(&array[start],&array[start+1],&base[start],&base[start+1],1);
  else {
    for(int i=0;i<n/2;i++) {
      cas_v2m(&array[start+i],&array[start+n/2+i],&base[start+i],&base[start+n/2+i],1);
    }
    bitonicSort(n/2,start,array,base);
    bitonicSort(n/2,start+n/2,array,base);
  }
}

static inline void merger(int n, int start, int* array, int* base) {
  if (n==2)
    cas_v2m(&array[start],&array[start+1],&base[start],&base[start+1],1);
  else {
    merger(n/2,start, array, base);
    merger(n/2,start+n/2, array, base);
    for(int i=0;i<n/2;i++) {
      cas_v2m(&array[start+i],&array[start+n-1-i],&base[start+i],&base[start+n-1-i],1);
    }
    bitonicSort(n/2,start,array,base);
    bitonicSort(n/2,start+n/2,array,base);
  }
}
void bubble_sort(int n, int* array, int* base)
{
  int i, j;
  for (j = 0; j < n - 1; j++)
    for (i = 0; i < n - 1 - j; i++)
    {	
      cas_v2m(&array[i],&array[i+1],&base[i],&base[i+1],1);
    }
}

void sortnet(long M_data_ref, long M_perm_ref) {
  int32_t* data = (int32_t*)M_data_ref;
  int32_t* perm = (int32_t*)M_perm_ref;
//  for(int i=0;i<N;i++) 
//    EPrintf("data[%d]=%d\n",i,data[i]);
//  for(int i=0;i<N;i++) 
//    EPrintf("perm[%d]=%d\n",i,perm[i]);
  merger(N,0,data,perm);
//  bubble_sort(N,data,perm);
//  for(int i=0;i<N;i++) 
//    EPrintf("data[%d]=%d\n",i,data[i]);
//  for(int i=0;i<N;i++) 
//    EPrintf("perm[%d]=%d\n",i,perm[i]);
}
