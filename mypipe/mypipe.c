#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
  int fds[2];
  if(pipe(fds) < 0){
    printf("pipe error!\n");
    return 1;
  }

  pid_t id = fork();
  if(id == 0){
    //write
    close(fds[0]);
    const char* str = "i am child!\n";
    int i = 5;
    while(i--){
      write(fds[1], str, strlen(str));
      printf("child write done!\n");
      sleep(1);
    }
    close(fds[1]);
    exit(0);
  }else{
    //read
    close(fds[1]);
    char buff[1024];
    while(1){
      ssize_t s = read(fds[0], buff, sizeof(buff)-1);
      if(s > 0){
        buff[s] = 0;
        printf("father recv done:%s", buff);
      }
      else if(s == 0){
        printf("child quit!\n");
        break;
      }
      else{

      }
    }
    waitpid(id, NULL, 0);
    close(fds[0]);
  }

  return 0;

}
