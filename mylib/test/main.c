#include <stdio.h>
#include "add.h"
#include "sub.h"

int main()
{
  printf("5 + 10 = %d\n", add(5, 10));
  printf("15 - 10 = %d\n", sub(15, 10));
  return 0;
}

