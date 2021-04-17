#include <stdio.h>
#include <unistd.h>
int main()
{
  int ret = syscall(335);
  printf("The return value is %d\n", ret);
  return 0;
}
