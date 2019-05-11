#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define FIFONAME "mypipe"

int main()
{
  int fd = open(FIFONAME, O_WRONLY);
  if(fd < 0){
    return 1;
  }

  char buff[1024];
  while(1){
    printf("Please Enter Your Message To Server# ");
    fflush(stdout);
    ssize_t s = read(0, buff, sizeof(buff)-1);
    buff[s-1] = 0;
    write(fd, buff, strlen(buff));
  }
  close(fd);
  return 0;
}
