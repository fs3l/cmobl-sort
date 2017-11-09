void swap(int* a, int* b)
{
  int temp = *a;
  *a = *b;
  *b = temp;
}

void swap_by_dir(int* a, int* b,int dir)
{
  if ((dir==1 && *a > *b) || (dir==0 && *a < *b)) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
  }
}



