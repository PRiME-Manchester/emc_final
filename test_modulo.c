#include <stdio.h>

int mod(int x, int m);

int main(void)
{
  int i;

  i = 0;
  printf("i:%2d %2d%%12=%d\n", i, i, i%12);

  i = 11;
  printf("i:%2d %2d%%12=%d\n", i, i, i%12);

  i = 12;
  printf("i:%2d %2d%%12=%d\n", i, i, i%12);

  i = 1;
  printf("i:%2d %2d%%12=%d\n", i, i, i%12);

  i = -1;
  printf("i:%2d %2d%%12=%d\n", i, i, i%12);

//////////////

  i = 0;
  printf("i:%2d %2d%%12=%d\n", i, i, mod(i,12));

  i = 11;
  printf("i:%2d %2d%%12=%d\n", i, i, mod(i,12));

  i = 12;
  printf("i:%2d %2d%%12=%d\n", i, i, mod(i,12));

  i = 1;
  printf("i:%2d %2d%%12=%d\n", i, i, mod(i,12));

  i = -1;
  printf("i:%2d %2d%%12=%d\n", i, i, mod(i,12));


  return 0;
}

int mod(int x, int m)
{
    return (x%m + m)%m;
}