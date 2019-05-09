#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX 1024

int main()
{
  char buf[MAX];
  char* myargv[16];
  
  
  while(1)
  {
    printf("[my@localhost haha]# ");
    //scanf("%s", buf);
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';
    int i = 0;
    myargv[i] = strtok(buf, " ");
    char* ret = NULL;
    while( ret = strtok(NULL, " ") )
    {
      myargv[++i] = ret;
    }
    myargv[++i] = NULL;

    //int j = 0;
    //for( ; j < i; j++ )
    //{
    //  printf("%s\n", myargv[j]);
    //}

    pid_t id = fork();
    if(id == 0)
    {
      execvp(myargv[0], myargv);
      exit(1);
    }
    else
    {
      waitpid(id, NULL, 0);
    }
  } 
  return 0;
}
