#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>

void Usage(std::string proc_)
{
  std::cout << "Uasge: " << proc_ << "server_ip server_port" << std::endl;
}

//client server_ip port
int main(int argc, char* argv[])
{
  if(argc != 3)
  {
    Usage(argv[0]);
    exit(1);
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock < 0)
  {
    std::cerr << "socket error!" << std::endl;
    exit(2);
  }
  
  struct sockaddr_in server;
  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(argv[2]));
  server.sin_addr.s_addr = inet_addr(argv[1]);
  
  std::string message;

  char buf[1024];
  for(;;)
  {
    socklen_t len = sizeof(server);
    std::cout << "Please Enter# ";
    std::cin >> message;
    sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr*)&server, len);
    ssize_t s = recvfrom(sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&server, &len);
    buf[s] = 0;
    std::cout << "Server echo# " << buf << std::endl;
  }
  return 0;
}
