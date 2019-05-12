#include "ThreadPool.hpp"
#include "Utils.hpp"

#define MAX_LISTEN 5
#define MAX_THREAD 5

class Sock{
private:
    int _listen_sock;
    int _port;

public:
    Sock(const int port) :_listen_sock(-1), _port(port)
    {}
    void Socket()
    {
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(_listen_sock < 0)
        {
            std::cerr << "socket error!" << std::endl;
            return;
        }

        int opt = 1;
        setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
    void Bind()
    {
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_port = htons(_port);
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0)
        {
            std::cerr << "bind error!" << std::endl;
            close(_listen_sock);
            return;
        }
    }
    void Listen()
    {
        if(listen(_listen_sock, MAX_LISTEN) < 0)
        {
            std::cerr << "listen error!" << std::endl;
            close(_listen_sock);
            return;
        }
    }
    int Accept()
    {
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        int sock = accept(_listen_sock, (struct sockaddr*)&peer, &len);
        if(sock < 0)
        {
            std::cerr << "accept error!" << std::endl;
            return -1;
        }
        return sock;
    }

    ~Sock()
    {
        close(_listen_sock);
    }
};


class HttpServer{
private:
    Sock _sock;
    ThreadPool *_tp;
public:
    HttpServer(const int &port) :_sock(port), _tp(NULL) {}

    bool InitHttpServer()
    {
        _sock.Socket();
        _sock.Bind();
        _sock.Listen();
        _tp = new ThreadPool(MAX_THREAD);
        if(_tp == NULL)
        {
            std::cerr << "thread pool malloc error!" << std::endl;
            return false;
        }
        _tp->ThreadPoolInit();

        return true;
    }

    //开始获取客户端新连接--创建任务，任务入队
    bool Start()
    {
        while(1)
        {
            int new_sock = _sock.Accept();
            if(new_sock < 0)
                continue;
            HttpTask ht(new_sock, HttpHandler);
            _tp->PushTask(ht);
        }
        return true;
    }

    static bool (HttpHandler)(int sock)  //http任务的处理函数
    {
			  HttpRequest rep(sock);    //接收并解析
			  HttpResponse rsp(sock);   //对请求做出响应
			  RequestInfo info;         //向rsp传递已解析的数据

			  if (rep.RecvHttpHeader(info) == false)   //接收http头部
        {
			  	  goto out;
			  }
        std::cout<<"---------------------RecvHttpHeader is ok!\n" << std::endl;
			  if (rep.ParseHttpHeader(info) == false)  //解析http头部
			  {
			  	  goto out;	
			  }
        std::cout<<"--------------------ParseHttpHeader is ok!\n" << std::endl;
			  if (info.RequestIsCGI(info))  //若当前请求为CGI请求，则执行CGI响应
        {
			  	  rsp.CGIHnadler(info);   
			  }
			  else                      //若当前请求不是CGI请求，则执行目录列表/文件下载响应
			  {
			  	  rsp.FileHandler(info);
			  }
         
        close(sock);
        return true;
    out:
        rsp.ErrHandler(info); //若是请求出错，返回一个错误响应
        close(sock);
        return false;
   }
};

int main(int argc, char* argv[])
{
    if(argc != 2)
        return -1;

    int port = atoi(argv[1]);
    signal(SIGPIPE, SIG_IGN);
    HttpServer* hsp = new HttpServer(port);
    hsp->InitHttpServer();
    hsp->Start();

    delete hsp;
    return 0;
}
