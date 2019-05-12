#pragma once 
#include "Utils.hpp"
#include <iostream>
#include <unordered_map>
#include <signal.h>                                                   
#include <stdlib.h>                               
#include <string>         
#include <vector>
#include <string.h>        
#include <unistd.h>                         
#include <netinet/in.h>      
#include <sys/types.h>                                
#include <sys/stat.h>
#include <sys/socket.h>        
#include <arpa/inet.h> 
#include <map>
#include <time.h>  
#include <errno.h>
#include <sstream> 
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <unordered_map>
#define WWWROOT "www"
#define MAX_GMT 128
#define MAX_HTTPHDR 4096
#define MAX_PATH 256
#define MAX_BUFF 4096

std::unordered_map<std::string, std::string> g_err_desc = {
    {"200", "OK"},
    {"206", "Partial Content"},
    {"400", "Bad Request"},
    {"403", "Forbidden"},
    {"404", "Not Found"},
    {"405", "Method Not Allowed"},
    {"413", "Request Entity Too Large"},
    {"500", "Internal Server Error"}
};

//文件的类型
std::unordered_map<std::string, std::string> g_mime_type = {
    {"txt", "application/octet-stream"},
    {"rtf", "application/rtf"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"avi", "video/x-msvideo"},
    {"gz",  "application/x-gzip"},
    {"tar", "application/x-tar"},
    {"mpg","video/mpeg"},
    {"au", "audio/basic"},
    {"midi","audio/midi"},
    {"ra", "audio/x-pn-realaudio"},
    {"ram", "audio/x-pn-realaudio"},
    {"midi","audio/x-midi"},
    {"html", "text/html"},
    {"unknow", "application/octet-stream"}
};

class Utils
{
public:
    //将数据按tag分隔
    static bool Split(std::string& src, const char* tag, std::vector<std::string>& v)
    {
        size_t index = 0;
        size_t pos = 0;
        while(index < src.length())
        {
            pos = src.find(tag, index);
            if(pos == std::string::npos)
                break;
            v.push_back(src.substr(index, pos - index));
            index = pos + strlen(tag);
        }
        if(index < src.length())
            v.push_back(src.substr(index, pos - index));

        return true;
    }
    
    static void TimeToGmt(time_t t, std::string& gmt)
    {
        //gmtime 将时间戳准换成格林威治时间， 返回一个结构体  
        //fomat 结构体 %a一周中的第几天  %d一个月中的第几天 ,%b月份的名称 %Y年份
        //strftime 将一个结构体中的时间按一定的格式进行转换，返回字符串的长度
        struct tm* mt = gmtime(&t);
        char tmp[MAX_GMT] = {0};
        int len = strftime(tmp, MAX_GMT-1, "%a, %d %Y %H:%M:%S GMT", mt);
        gmt.assign(tmp, len);
    }

    static void DigitToStr(std::string& str, int64_t num)
    {
        std::stringstream ss;
        ss << num;
        str = ss.str();
    }
    static int64_t StrToDigit(const std::string &str)
    {
        int64_t num;
        std::stringstream ss;
        ss << str;
        ss >> num;
        return num;
    }

    static void MakeETag(int64_t ino, int64_t size, int64_t mtime,
        std::string& etag)
    {
        //"ino-size-mtime"
        std::stringstream ss;
        ss << "\"" << std::hex<<ino << "-" << std::hex<<size << "-" << std::hex<<mtime << "\"";
        etag = ss.str();
    }
    //获取文件类型
    static void GetMime(const std::string &file, std::string &mime)
    {
        size_t pos;
        pos = file.find_last_of("."); //找文件名中的最后一个.
        if(pos == std::string::npos)
        {
            mime = g_mime_type["unknow"];
            return;
        }
        std::string suffix = file.substr(pos + 1);
        auto it = g_mime_type.find(suffix);
        if(it == g_mime_type.end())
        {
            mime = g_mime_type["unknow"];
        }
        else
        {
            mime = it->second;
        }
    }
};
struct RequestInfo 
{
    //包含HttpRequest解析出的请求信息
	  std::string _method;//请求方法
	  std::string _version;//协议版本
	  std::string _path_info;//资源路径
	  std::string _path_phys;//资源实际路径
	  std::string _query_string;//查询字符串
    std::unordered_map<std::string, std::string> _header_list;//整个头部信息中的键值对
	  struct stat _st;
	  std::string _err_code;  //存放错误号404  400
    int _part;
    std::vector<std::string> _partlist;


    bool RequestIsCGI(RequestInfo& info)  //判断请求类型
    {
        if(((info._method == "GET") && !info._query_string.empty())
            || info._method == "POST")
            return true;
        return false;
    }
};


class HttpRequest
{
    //http数据的接收接口
    //http数据的解析接口
    //对外提供能够获取处理结果的接口
    private:
        int  _cli_sock;
        std::string _http_header;
    public:
        HttpRequest(int sock):_cli_sock(sock) {}

        //接收http请求头
        bool RecvHttpHeader(RequestInfo &info)
        {
            //从_cli_sock的缓冲区中读数据，只想读取http头部信息，所以在读取数据时只要读取到两个'\r\n'就可以了，使用时recv只需用一个选项MSG_PEEK
            char tmp[MAX_HTTPHDR];
            while(1)
            {
                int ret = recv(_cli_sock, tmp, MAX_HTTPHDR, MSG_PEEK); //并不从缓冲区冲拿出，只是读取
                if(ret <= 0)
                {
                    if(errno == EINTR || errno == EAGAIN)
                    {
                        //EINTR  表示被信号打断
                        //ENGAIN 非阻塞时，缓冲区中没有错误
                        continue;
                    }
                    info._err_code = "500";//服务器被中断，出错
                      return false;
                  }
                  char *ptr = strstr(tmp, "\r\n\r\n");
                  if(ptr == NULL)  //没找到
                  {
                      if(ret == MAX_HTTPHDR) //发送的头部太大
                      {
                          info._err_code = "413";
                          return false;
                      }
                      else if(ret < MAX_HTTPHDR) //可能还没读完
                      {
                          usleep(1000); 
                          continue;
                      }
                  }
                  int hdr_len = ptr - tmp; //头部长度
                  _http_header.assign(tmp, hdr_len); //截取字符串
                  recv(_cli_sock, tmp, hdr_len+4, 0); //从缓冲区中移除头部
                  std::cout << "header: \n" << _http_header.c_str() << std::endl;
                  break;
              }
              return true;
          }

          //解析http请求头
          bool ParseHttpHeader(RequestInfo& info)
          {
              //请求头解析  
              //请求方法 url 协议版本\r\n
              //key: val\r\nkeyval
              
              std::vector<std::string> v_hander; // 将header按行放入数组中
              if(!Utils::Split(_http_header, "\r\n", v_hander))
              {
                  std::cerr << "split error!" << std::endl;
              }
              
              for(size_t i = 0; i < v_hander.size(); i++)
              {
                  if(i == 0)
                  {
                      //处理首行
                      ParseFristLine(info, v_hander[i]);
                  }

                  size_t pos = v_hander[i].find(": ");
                  // k v
                  info._header_list[v_hander[i].substr(0, pos)] = v_hander[i].substr(pos + 2);
              }
              
              return true;
          }

      private:
          bool ParseFristLine(RequestInfo& info, std::string& first_line)
          {
              std::vector<std::string> v_fline;
              Utils::Split(first_line, " ", v_fline);
              if(v_fline.size() != 3)
              {
                  info._err_code = "400";  
                  return false;
              }

              info._method = v_fline[0];
              std::string url = v_fline[1]; 
              info._version = v_fline[2];

              if(info._method != "GET" && info._method != "POST" && info._method != "HEAD")
              {
                  info._err_code = "405";
                  return false;
              }
              if(info._version != "HTTP/1.0" && info._version != "HTTP/1.1")
              {
                  info._err_code = "400";
                  return false;
              }

              size_t pos = url.find("?");
              if(pos == std::string::npos)
              {
                  info._path_info = url;
              }
              else 
              {
                  info._path_info = url.substr(0, pos);
                  info._query_string = url.substr(pos + 1);
              }
              info._path_phys = WWWROOT + info._path_info;

              
              return PathIsLegal(info);
          }
          
          //判断路径是否合法  
          bool PathIsLegal(RequestInfo& info)
          {
              //stat输出的文件信息，通过他的返回值判断文件是否存在  
              //Linux 文件名的最大长度为256 是一个宏
              //stat获取状态信息出错，表示无法访问到文件
              if(stat(info._path_phys.c_str(), &info._st) < 0)
              {
                  info._err_code = "404";
                  return false;
              }

              //realpath 将一个路径转换成绝对路径  
              //缺点：请求路径不存在，会造成段错误,先判断再转换
              char tmp[MAX_PATH] = {0};
              realpath(info._path_phys.c_str(), tmp);  

              info._path_phys = tmp; 
              if(info._path_phys.find(WWWROOT) == std::string::npos)
              {
                  info._err_code = "403";
                  return false;
              }      

              return true;
          }
  };


  class HttpResponse
  {
      //文件请求（完成文件下载/列表功能）接口
      //CGI请求接口
      private:
          int _cli_sock;
          //Etag: "inode-mtime-fsize"\r\n 
          std::string _etag;//表明文件是否修改过 假如浏览器已经下载过的，就不再返回了(304)，通过以下的属性来判断
          std::string _mtime;//文件的最后一次修改时间
          std::string _date;//系统的响应时间 
          std::string _fsize;//文件大小
          std::string _mime;//文件类型
      private:
          std::string GetErrDesc(std::string code) 
          {
              auto it = g_err_desc.find(code);
              if(it == g_err_desc.end())
              {
                  return "Unknow Error";
              }
              return it->second;
          }

          bool IsDir(RequestInfo &info)
          {
              if(info._st.st_mode & S_IFDIR)
              {
                  if(info._path_info.back() != '/')
                  {
                      info._path_info.push_back('/');
                  }
                  if(info._path_phys.back() != '/')
                  {
                      info._path_phys.push_back('/');
                  }
                  return true;
              }
              return false;
          }

          bool SendData(const std::string& buf)
          {
              if(send(_cli_sock, buf.c_str(), buf.length(), 0) < 0)
              {
                  return false;
              }
              return true;
          }

          bool SendCData(const std::string& buf) //分块发送
          {
              if(buf.empty())
              {
                  return SendData("0\r\n\r\n");
              }
              std::stringstream ss;
              ss <<  std::hex << buf.length() << "\r\n";
              SendData(ss.str());
              SendData(buf);
              SendData("\r\n");
              return  true;
          }
      public:
          HttpResponse(int sock):_cli_sock(sock) {}

          bool InitResponse(RequestInfo req_info) //初始化一些请求的响应信息
          {
              //Last-Modified:
              Utils::DigitToStr(_mtime, req_info._st.st_mtime);
              //ETag:
              Utils::MakeETag(req_info._st.st_ino, req_info._st.st_size, req_info._st.st_mtime, _etag); 
              //Date:
              time_t t = time(NULL); 
              Utils::TimeToGmt(t, _date);
              //文件大小
              Utils::DigitToStr(_fsize, req_info._st.st_size);
              //mime
              Utils::GetMime(req_info._path_phys.c_str(), _mime);
              return true;
          }

          bool IsPartDownload(RequestInfo& info)
          {
              std::cout<<"---------------------------------In IsPartDownload:\n";
              auto it = info._header_list.find("If-Range");
              if(it == info._header_list.end())
              {                                                            
                  return false;                                                                                       
              }
              else 
              {
                  if(it->second == _mtime||it->second == _etag);
                  else 
                      return false;        
              }
              it = info._header_list.find("Range");
              if(it == info._header_list.end())
              {
                  return false;                
              }
              else 
              {
                  std::string range = it->second;
                  std::cout<<"--------------------In IsPartDownload: range:"<<range<<"\n";
                  info._part = Utils::Split(range,", ",info._partlist); 
                  return true;                                                                                                                      
              }              
          }
          bool ProcessPartDownload(RequestInfo &info,int i)
          {
              std::cout<<"---------------------------In Part Download\n";
              std::cout<<info._partlist[i]<<"\n";
              std::string range = info._partlist[i];
              if(i == 0)
              {
                  range.erase(range.begin(),range.begin()+6);
                  std::cout<<"Range: "<<range<<"\n";
              }
              size_t pos = range.find("-");
              int64_t start = 0;
              int64_t end = 0;
              if(pos == 0)
              {
                  end = Utils::StrToDigit(_fsize)-1;
                  start = end-Utils::StrToDigit(range.substr(pos+1));                                                                  
              }
              else if(pos == range.size()-1)
              {
                  end = Utils::StrToDigit(_fsize)-1;
                  range.erase(pos,1);
                  start = Utils::StrToDigit(range);
              }
              else
              {
                  start = Utils::StrToDigit(range.substr(0,pos));
                  end = Utils::StrToDigit(range.substr(pos+1));
              }
              std::cout<<"----------------PartDownloading: start = "<<start<<"  end = "<<end<<"\n";
              std::string rsp_header; 
              rsp_header = info._version + " 206 PARTIAL CONTENT\r\n";
              rsp_header += "Content-Type: " + _mime + "\r\n";
              //?????????????????????????
              std::string len;
              Utils::DigitToStr(len, end-start+1);                                            
              std::string start_;
              Utils::DigitToStr(start_, start);                                            
              std::string end_;
              Utils::DigitToStr(end_, end);                                            
              rsp_header += "Connection: close\r\n";
              rsp_header += "Content-Range: bytes " + start_ + "-" + end_ + "/" + _fsize + "\r\n";
              rsp_header += "Content-Length: " + len + "\r\n";
              rsp_header += "Accept-Ranges: bytes\r\n"; 
              if(info._partlist.size()>1)
              {
                  rsp_header += "Content-Type: multipart/byteranges\r\n";
              }
              rsp_header += "ETag: " + _etag + "\r\n";
              rsp_header += "Last-Modified: " + _mtime + "\r\n";
              rsp_header += "Date: " + _date + "\r\n\r\n";
              SendData(rsp_header);
              std::cout<<"------PartDownloading...   rsp_header:\n"<<rsp_header;
              int fd = open(info._path_phys.c_str(), O_RDONLY);
              if (fd < 0)
              {
                  info._err_code = "400";
                  ErrHandler(info);
                  return false;
              }
              lseek(fd,start,SEEK_SET);
              int64_t slen = end-start+1;
              int64_t rlen;                                                                                                                           
              char tmp[MAX_BUFF];
              int64_t flen = 0;
              while ((rlen = read(fd, tmp, MAX_BUFF)) > 0)
              {
                  if(flen+rlen>slen)
                  {
                      send(_cli_sock, tmp, slen-flen, 0);
                      break;
                  }
                  else 
                  {
                      flen+=rlen;
                      send(_cli_sock,tmp,rlen,0);
                  }
              }
              close(fd);
              return true;
          }

          bool ProcessFile(RequestInfo &info)     //文件下载功能
          {
              std::string rsp_header;
              rsp_header = info._version + " 200 OK\r\n";
              rsp_header += "Content-Type: " + _mime + "\r\n";
              rsp_header += "Connection: close\r\n";
              rsp_header += "ETag: " + _etag + "\r\n";
              rsp_header += "Content-Length: " + _fsize + "\r\n";
              rsp_header += "Last-Modifoed: " + _mtime += "\r\n";
              rsp_header += "Date: " + _date + "\r\n\r\n";
              SendData(rsp_header);

              int fd = open(info._path_phys.c_str(), O_RDONLY);
              if(fd < 0)
              {
                  info._err_code = "400";
                  ErrHandler(info);
              }

              int rlen = 0;
              int tmp[MAX_BUFF];
              while((rlen = read(fd, tmp, MAX_BUFF)) > 0)
              {
                  //不能用string 或char*来发送数据 ，因为可能会在文件中有0  //可能会造成对端关闭
                  if(send(_cli_sock, tmp, rlen, 0) < 0)
        					{
          						fprintf(stdout,"send error");
        					} 
              }

              close(fd);

              //md5sum 对文件进行验证
              
              //断点续传:
              // 请求：
              // IF- range : etag 
              // Range: bytes=1-100  保存数据请求的范围
              // 响应：
              // 分块传输使用206  不能用200 ，200返回完整的内容
              // 200:Accept-Ranges: bytes  可以接收分块传输，接受的单位为bytes
              // 206:
              // Content-Range: bytes 101-200/1000
              // Content-Length: 100
                                            
              return true;
          }
          bool ProcessList(RequestInfo &info)     //文件列表功能
          {
              //组织头部：
              // 首行
              // Content-Type: text/html\r\n
              // ETag: \r\n
              // Date: \r\n
              // Last-Modifoed: \r\n
              // Transfer-Encoding: chunked\r\n  //分块传输（1.1版本）
              // Connection: close\r\n\r\n
              //正文：
              // 每一个目录下的文件都要组织一个html标签信息
              
              std::string rsp_header;
              rsp_header = info._version + " 200 OK\r\n";
              rsp_header += "Content-Type: text/html\r\n";
              rsp_header += "Connection: close\r\n";
              if(info._version == "HTTP/1.1")
              {
                  rsp_header += "Transfer-Encoding: chunked\r\n";
              }
              rsp_header += "ETag: " + _etag + "\r\n";
              rsp_header += "Last-Modifoed: " + _mtime += "\r\n";
              rsp_header += "Date: " + _date + "\r\n\r\n";
              SendData(rsp_header);
  
              std::string rsp_body;
              rsp_body = "<html><head>";
              rsp_body += "<meta charset='UTF-8'>"; 
              rsp_body += "<title>共享文件服务器</title>";
              rsp_body +="</head><body><h1> 当前文件路径: " + info._path_info + "</h1>";
              rsp_body += "<form action='/upload' method='POST' enctype='multipart/form-data'>";
              rsp_body += "<input type='file' name='FileUpload' />";
              rsp_body += "<input type='submit' value='上传' />";
              rsp_body += "</form>";
              rsp_body += "<hr /><ol>";
              SendCData(rsp_body);
  
              //获取目录下的每一个文件，组织处html信息，chunke传输
              //scandir 判断目录下有哪些文件  返回值为当前文件数 
              //struct 中有不带路径的文件名列表  取出文件名后 与当前目录进行组合，通过stat获取文件信息  第三个为0 表示不过滤 
              //filt(struct *dirent )
              //{
              //	过滤掉 . 目录   
              //  if(strcmp(dirent->d->_name, "."))
              //    	return 1;
              //  else 
              //      return 0;
              //}
              struct dirent **p_dirent = NULL;
  						int  num = scandir(info._path_phys.c_str(), &p_dirent, 0, alphasort);
        			for(int i = 0; i < num ; ++i)
  						{
          				std::string file_html;
          				std::string file = info._path_phys + p_dirent[i]->d_name;  //当前文件的全路径
          				struct stat st;
          				if(stat(file.c_str(), &st) < 0)
          				{
          				  	continue;
          				}
  								//已经获取到此文件，该组织了
          				std::string mtime;
          				Utils::TimeToGmt(st.st_mtime, mtime);
  
          				std::string mime;
          				Utils::GetMime(file, mime);
  
          				std::string filesize;
          				Utils::DigitToStr(filesize, st.st_size / 1024.0); //kb
  
          				file_html += "<li><strong><a href='";
          				file_html += info._path_info; //加粗的链接
          				file_html += p_dirent[i]->d_name ;
          				file_html += "'>"; 
          				file_html += p_dirent[i]->d_name ;
          				file_html += "</a></strong>";
          				file_html += "<br /><small>";
          				file_html += "modified: " + mtime + "<br />";
          				file_html += mime + "-" + filesize + "kbytes";
          				file_html += "<br /><br /></small></li>";
          				SendCData(file_html);
        			}
  
        			rsp_body = "</ol><hr /></body></html>";
        			SendCData(rsp_body);
        			SendCData("");
              return true;
          }

  		    bool ProcessCGI(RequestInfo &info)      //cgi请求处理
          {
              //使用外部程序完成cgi请求处理结果---文件上传
              //将http头信息和正文全部交给子进程处理
              //使用环境变量传递头信息
              //使用管道传递正文数据
              //使用管道接收cgi程序的处理结果
              //流程：创建管道，创建子进程，设置子进程环境变量，程序替换

            	int in[2];//从父进程向子进程传递正文数据
			       	int out[2]; //子进程向父进程传递结果
			 
			       	if((pipe(in) < 0) || (pipe(out) < 0)){
			       	  	info._err_code = "500";
			       	  	ErrHandler(info);
			       	  	return false;
			       	}
			 
			       	pid_t pid;
			       	pid = fork();
			       	if(pid < 0)
							{
			       	  	info._err_code = "500";
			       	  	ErrHandler(info);
			       	  	return false;
			       	}
							else if(pid == 0)
							{
			       	  	//设置请求行数据
			       	  	setenv("METHOD", info._method.c_str(), 1);
			       	  	setenv("PATH_INFO", info._path_info.c_str(), 1);
			       	  	setenv("VERSION", info._version.c_str(), 1);
			       	  	setenv("QUERY_STRING", info._query_string.c_str(), 1);
			 
			       	  	//传递首部字段
			       	  	for(auto it = info._header_list.begin(); it != info._header_list.end(); it++)
									{
			       	  	  	setenv(it->first.c_str(), it->second.c_str(), 1);
			       	  	}
			 
			       	  	close(in[1]);
			       	  	close(out[0]);
			 
			       	  	//程序替换后文件描述符就会消失，子进程将运行自己的代码，所以需要进行重定向到标准输入输出
			       	  	dup2(in[0], 0);//子进程将从标准输入读取数据
			       	  	dup2(out[1], 1);//子进程直接打印处理结果传递给父进程
			 
			       	  	execl(info._path_phys.c_str(), info._path_phys.c_str(), NULL);
			       	  	exit(0);
			 
			       	}
			 
			       	//父进程关闭in[0] 与 out[1]
			       	close(in[0]);
			       	close(out[1]);
			 
			       	//1、父进程向子进程传递正文数据通过in管道，
			       	auto it = info._header_list.find("Content-Length"); 
							//如果没有正文就不传递
			       	if(it != info._header_list.end()){
			       	  	char buf[MAX_BUFF] = {0};
			       	  	int64_t cont_len = Utils::StrToDigit(it->second);
			 
			       	  	//只读了一遍可能没读完
			       	  	int tlen = 0;
			       	  	while(tlen < cont_len){
			       	    //为了防止后面还有数据，造成粘包， 和所能接收到的数据进行比较有多少接多少， 最多接收到缓冲区满
			       	    		int len = MAX_BUFF > (cont_len -tlen ) ?(cont_len - tlen) : MAX_BUFF;
			       	    		int rlen = recv(_cli_sock, buf, len, 0);
			       	    		if(rlen <= 0)
			       	      			return false;
			 
			 
			       	    		//向管道写入数据会卡住，有大小限制， 管道满了就会阻塞
			       	    		if(write(in[1], buf, rlen) < 0)
			       	      			return false;
			 
			       	    		tlen += rlen;
			       	  	}
			       	}
			 
			       	//组织响应头部       
			       	std::string rsp_header;
			       	rsp_header = info._version + " 200 OK\r\n"; rsp_header += "Connection: close\r\n";
			       	rsp_header += "Content-Type: text/html\r\n";
			       	rsp_header += "ETag: " + _etag + "\r\n";
			       	rsp_header += "Last-Modified: " + _mtime + "\r\n";
			       	rsp_header += "Date: " + _date + "\r\n\r\n";
			       	SendData(rsp_header);
			 
			       	while(1){
			       	  	//2、从子进程读取所返回的结果
			       	  	char buf[MAX_BUFF] = {0};
			       	  	int rlen = read(out[0], buf, MAX_BUFF);
			       	  	if (rlen == 0)
			       	  	  	break;
			 
			       	  	send(_cli_sock, buf, rlen, 0);
			       	}
			 
			       	close(in[1]);
			       	close(out[0]);
			     
			       	//处理错误
 
              return true;
          }
  		
          //为用户提供的接口
          
  		    bool ErrHandler(RequestInfo &info)      //处理错误响应
          {
              //根据info中的_err_code和错误描述组织一个http头部进行响应
              
              //首行：协议版本 状态码 状态描述\r\n
              //头部：Content_Length Date
              //空行：
              //正文（错误响应一般没有正文）：rep_body = "<html><body><h1>404;<h1></body></html>"
              
              std::string rsp_body; //正文
              rsp_body = "<html><body><h1>" + info._err_code + "<h1></body></html>";
  
              std::string rsp_hander; //首部
              time_t t = time(NULL);
              std::string gmt;
              Utils::TimeToGmt(t, gmt);
              std::string body_length;
              Utils::DigitToStr(body_length, rsp_body.length());
              rsp_hander = info._version + " " + info._err_code + " " + GetErrDesc(info._err_code) + "\r\n";
              rsp_hander += "Date: " + gmt + "\r\n";
              rsp_hander += "Content_Length: " + body_length + "\r\n\r\n";
  
              std::cout << "response:" <<std::endl;
              std::cout << rsp_hander << rsp_body <<std::endl;
              send(_cli_sock, rsp_hander.c_str(), rsp_hander.length(), 0);
              send(_cli_sock, rsp_body.c_str(), rsp_body.length(), 0);
              return true;
          }
          
  		    bool CGIHnadler(RequestInfo &info)
  		    {
  			      InitResponse(info); //初始化cgi响应信息
  			      ProcessCGI(info);   //执行cgi响应
              return true;
  		    }
  
  		    bool FileHandler(RequestInfo &info)
  		    {
  			      InitResponse(info); //初始化文件响应信息
  			      if (IsDir(info)) //判断请求文件是否为目录
  			      {
  				        ProcessList(info); //执行文件列表响应信息 
  			      }
              else 
              {
                  //执行文件下载
                  if(IsPartDownload(info))
                  {
                      for(int i = 0; i < info._part; i++)
                      {
                          ProcessPartDownload(info, i);
                      }
                  }
                  else 
                      ProcessFile(info); 
              }
              return true;
  		    }

};

