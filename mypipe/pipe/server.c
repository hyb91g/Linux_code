#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define FIFONAME "mypipe"

int main()
{
  mkfifo(FIFONAME, 0644);
  int fd = open(FIFONAME, O_RDONLY);
  if(fd < 0){
    return 1;
  }

  char buff[1024];
  while(1){
    ssize_t s = read(fd, buff, sizeof(buff)-1);
    if(s > 0)
    {
      buff[s] = 0;
      printf("client# %s\n", buff);
    }
    else if(s == 0)
    {
      printf("client quit!\n");
      break;
    }
    else
    {
      break;
    }
  }

  close(fd);
  return 0;
}
