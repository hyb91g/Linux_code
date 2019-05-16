#include "UdpServer.hpp"
void Usage(std::string proc_)
{
  std::cout << "Uasge: " << proc_ << "ip port" << std::endl;
}

//server ip port
int main(int argc, char* argv[])
{
  if(argc != 3)
  {
    Usage(argv[0]);
    exit(1);
  }
  UdpServer *sp = new UdpServer(argv[1], atoi(argv[2]));
  sp->InitServer();
  sp->Start();
  delete sp;
  return 0;
}
