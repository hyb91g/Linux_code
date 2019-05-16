#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
class Sock
{
private:
  int sock;
  std::string ip;
  int port;
public:
  Sock(std::string &ip_, int &port_) :ip(ip_), port(port_)
  {}

  void Socket()
  {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
      std::cout << "socket error!" << std::endl;
      exit(2);
    }
  }

  void Bind()
  {
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());
    int b = bind(sock, (struct sockaddr*)&server, sizeof(server));
    if(b < 0)
    {
      std::cout << "bind error!" << std::endl;
      exit(3);
    }
  } 
  void Recv(std::string &str_, struct sockaddr_in &peer, socklen_t &len)
  {
    char buf[1024];
    len = sizeof(sockaddr_in);
    ssize_t s = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&peer, &len);
    if(s > 0)
    {
      buf[s] = 0;
      str_ = buf;
    }
  }
  void Send(std::string &str_, struct sockaddr_in &peer, socklen_t &len)
  {
    sendto(sock, str_.c_str(), str_.size(), 0, (struct sockaddr*)&peer, len);
  }
  ~Sock()
  {
    close(sock);
  }

};

class UdpServer{
private:
  Sock sock;
public:
  UdpServer(std::string ip_, int port_) :sock(ip_, port_)
  {}
  void InitServer()
  {
    sock.Socket();
    sock.Bind();
  }
  void Start()
  {
    struct sockaddr_in peer;
    socklen_t len;
    std::string message;
    for(;;)
    {
      sock.Recv(message, peer, len);
      std::cout << "[" << inet_ntoa(peer.sin_addr) << ":" << ntohs(peer.sin_port) << "]# " << message << std::endl;
      message += " server";
      sock.Send(message, peer, len);
      std::cout << "server echo success!" << std::endl;
    }
  }
  ~UdpServer()
  {}

};
