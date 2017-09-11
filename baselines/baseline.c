#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define N 32768

#define repeat 10


void swap(int *a,int *b)
{       
  int temp=*a;
  *a=*b;
  *b=temp;
}

void permutation_generate(int* permutation, int n)
{       
  int i;
  for(i=0;i<=n-2;i++){
    int j=rand()%(n-i);
    swap(&permutation[i],&permutation[i+j]);
    }
}
void permutation_print(int32_t *p,int size)
{
  printf("perm:\n");
  int i;
  for(i=0;i<size;i++)
    printf("perm[%d]=%d\n",i,p[i]);
}
void data_print(int32_t *data,int size)
{
  printf("data:\n");
  int i;
    for(i=0;i<size;i++)
      printf("data[%d]=%d\n",i,data[i]);
}
void output_print(int32_t *output,int size)
{
    printf("output:\n");
      int i;
          for(i=0;i<size;i++)
                  printf("output[%d]=%d\n",i,output[i]);
}


void transform(int32_t* p,int32_t* matrix,int n)
{
  int i;
  for(i=0;i<n;i++)
  {
    matrix[p[i]*n+i]=1; 
  }
}

void matrix_print(int32_t* a,int n)
{
  printf("matrix:\n");
  for(int i=0;i<n;i++)
  {
    for(int j=0;j<n;j++)
      printf("%3d",a[i*n+j]);
    printf("\n");
  }
}

int32_t matrix[N*N];
int main()
{
  int i,j;

  int32_t perm[N];
  memset(perm,0,sizeof(int)*N);
  for(i=0;i<N;i++)
    perm[i]=i;
  //permutation_print(perm,N);
  permutation_generate(perm,N);
  //permutation_print(perm,N);

  memset(matrix,0,sizeof(int)*N*N);

  //matrix_print(matrix,N);
  transform(perm,matrix,N);
  //matrix_print(matrix,N);

  int32_t data[N];
  for(i=0;i<N;i++)
    data[i]=i;
  //data_print(data,N);
  
  int32_t output[N];

  double start,finish;
  start=clock();
  for(int m=0;m<repeat;m++)
  {
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
      output[i]=data[x];
    }
  }
  finish=clock();
  output_print(output,N);
  printf("%f seconds\n",((finish-start)/repeat)/CLOCKS_PER_SEC);
  return 0;

}
