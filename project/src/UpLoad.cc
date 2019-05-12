#include "Utils.hpp"
enum _boundry_type
{
    BOUNDRY_NO = 0,
    BOUNDRY_FIRST,
    BOUNDRY_MIDDLE,
    BOUNDRY_LAST,
    BOUNDRY_PART   //若半个boundry
};

class UpLoad
{
    //CGI外部程序中的文件上传功能处理接口
    
    //流程：
    // InitUploadInfo:初始化文件上传信息
    //    1.从Content-type 获取boundry内容
    //    2.组织first/middl/last boudry 信息
    //  
    // ProcessUpload: 处理文件上传
    //    1.从正文起始位置匹配first boundry
    //    2.从boundry 头信息获取上传的文件名称
    //    3.继续循环从剩下的信息中匹配middle boundry(可能会有多个middle boundry) 
    //      a. middle boundry 之前的数据是上一个文件的数据
    //      b.将之前的数据存储到文件中关闭文件
    //      c.从头信息中获取文件名，若有， 则打开文件
    //    4.当匹配到last boundry时
    //      a.将boundry 之前的数据存储到文件
    //      b.文件上传处理完毕

private:
		int _file_fd; //遇到文件名就将那个文件描述符打开
    int64_t _content_len; //正文长度
	  bool _is_store_file;
	  std::string _file_name;
	  std::string _first_boundry;
	  std::string _middle_boundry;
	  std::string _last_boundry;
private:
    //只要匹配带一个\r\n都假设他有可能是middle_boundary，blen(buffer的长度)匹配的长度
    int MatchBoundey(char *buf, size_t blen, int *boundry_pos) //匹配boundry
    {
        //----boundary 
        //first_boundary:   ------boundary 
        //middle_boundary:  \r\n------boundary\r\n 
        //last_boundary:    \r\n------boundary--       
        //middle_boundary和last_boundary的长度是一样的
        
        //从起始位置匹配_first_bondry
        if (!memcmp(buf, _first_boundry.c_str(), _first_boundry.length()))
        {
            *boundry_pos = 0;
            return BOUNDRY_FIRST;
        }

        //到这里匹配的是middle_boundary和last_boundary
        //匹配原则：如果剩余的长度大于boundary的长度就进行完全匹配，否则进行部分匹配
        for(size_t i = 0; i < blen; i++)
        {
            //字符串剩余长度大于boundry的长度，则全部匹配
            if((blen - i) >= _middle_boundry.length())
            {
                if (!memcmp(buf+i, _middle_boundry.c_str(), _middle_boundry.length()))
                {
                    *boundry_pos = i;
                    return BOUNDRY_MIDDLE;
                }
                if (!memcmp(buf+i, _last_boundry.c_str(), _last_boundry.length()))
                {
                    *boundry_pos = i;
                    return BOUNDRY_LAST;
                }
            }
            //字符串剩余长度小于boundry的长度，则部分匹配，防止出现半个boundry
            else
            {
                int cmp_len = blen - i;

                if (!memcmp(buf+i, _middle_boundry.c_str(), cmp_len))
                {
                    *boundry_pos = i;
                    return BOUNDRY_PART;
                }
                if (!memcmp(buf+i, _last_boundry.c_str(), cmp_len))
                {
                    *boundry_pos = i;
                    return BOUNDRY_PART;
                }
            }
        }
        return BOUNDRY_NO;
    }
    bool GetFileName(char *buf, int *content_pos)
    {
        char *ptr = NULL;
        //查看是否是个完整的上传文件的头部，有没有\r\n\r\n
        ptr = strstr(buf, "\r\n\r\n");
        if(ptr == NULL)
        {
            *content_pos = 0;
            return false;
        }
        *content_pos = ptr - buf + 4;

        //将http上传文件的头部信息拿出来
        std::string header;
        header.assign(buf, ptr - buf);

        std::string file_sep = "filename=\"";
        size_t pos = header.find(file_sep);
        if(pos == std::string::npos)
        {
            return false;
        }
        std::string file_name = header.substr(pos + file_sep.length());
        pos = _file_name.find("\"");
        if(pos == std::string::npos)
        {
            return false;
        }
        //从文件名后面的"删除到结尾"
        _file_name.erase(pos);
        //如果直接使用WWWROOT进行拼接获取文件所在路径和名字的时候这个时候就会每次上传的文件都在www目录下，不会发生改变
        //所以要使用实际路径在加上文件名就好了
        _file_name = WWWROOT;
        _file_name += "/";
        _file_name += file_name;
        return true;
    }
    bool CreateFile()
    {
        _file_fd = open(_file_name.c_str(), O_CREAT|O_WRONLY, 0664);
        if(_file_fd < 0)
        {
            fprintf(stderr, "open error:%s \n", strerror(errno));
            return false;
        }
        return true;
    }
    bool CloseFile()
    {
        if(_file_fd != -1)
        {
            close(_file_fd);
            _file_fd = -1;
        }
        return true;
    }
    bool WriteFile(char *buf, int len)
    {
        if (_file_fd != -1)
        {
            write(_file_fd, buf, len);               
        }
        return true;
    }

public:
    UpLoad()
        : _file_fd(-1) 
    {}
	  bool InitUpLoadInfo() //初始化上传文件信息：正文长度
    {
        //1.从Content-type 获取boundry内容
        //2.组织first/middl/last boudry 信息
        
        char *ptr = getenv("Content-Length");
        if(ptr == NULL)
        {
            fprintf(stderr, "have no Content-Length\n");
            return false;
        }
        _content_len = Utils::StrToDigit(ptr);

        ptr = getenv("Content-Type");
        if(ptr == NULL)
        {
            fprintf(stderr, "have no Content-Type\n");
            return false;
        }
        std::string boundry_sep = "boundry=";
        std::string content_type = ptr;
        size_t pos = content_type.find(boundry_sep);
        if(pos == std::string::npos)
        {
            fprintf(stderr, "contendt_type have no boundry\n");
            return false;
        }
        std::string boundry = content_type.substr(pos + boundry_sep.length());
        _first_boundry = "--" + boundry;
        _middle_boundry = "\r\n" + _first_boundry + "\r\n";
        _last_boundry = "\r\n" + _first_boundry + "--";
        return true;
    }

	  bool ProcessUpLoad() //对正文进行处理，将文件数据进行存储（处理文件上传）
    {
        //tlen : 当前已经读取的长度
        //blen : buffer长度
        int64_t tlen = 0, blen = 0;
        char buf[MAX_BUFF];
        while(tlen < _content_len)
        {
            //从管道中将数据读出来
            int len = read(0, buf + blen, MAX_BUFF - blen);
            blen += len; //当前buf中数据的长度
            int boundry_pos; //分隔符下标 
            int content_pos; //文件名的开始下标

            int flag = MatchBoundey(buf, blen, &boundry_pos);
            if(flag == BOUNDRY_FIRST)
            {
                //从boundry头中获取文件名
                //若获取文件名成功，则创建文件， 打开文件
                //将头信息从buf中移除，剩下的数据进行下一步匹配
                if (GetFileName(buf, &boundry_pos))
                {
                    CreateFile();
                    //buf里面数据的长度剪短
                    blen -= _content_len;
                    //匹配到了就把内容去除,将从数据内容到结尾的数据全部向前移动覆盖前面的数据，第三个参数是blen，因为blen在前面已经减过了
                    memmove(buf, buf + content_pos, blen);
                    memset(buf + blen, 0, content_pos);
                }
                else 
                {
                    //有可能不是上传文件，没有filename所以匹配到了_first_boundary也要将其去掉
                    //没有匹配成功就把boundary分隔符的内容去除，因为此时的content_pos的位置没有找到呢
                    blen -= boundry_pos;
                    memmove(buf, buf + boundry_pos, blen);
                    memset(buf + blen, 0, boundry_pos);
                }
            }

            while(1)
            {
                //没有匹配到middle分隔符，就跳出循环 
                flag = MatchBoundey(buf, blen, &boundry_pos);
                if(flag != BOUNDRY_MIDDLE)
                {
                    break;
                } 
                //匹配middle_boundry成功：
                //将boundry之前的数据写入文件，将数据从buf中移除
                //关闭文件
                //看boundry头中是否有文件名—雷同first_boundry
                WriteFile(buf, boundry_pos);//如果有文件打开就进行写入，没有就不进行写入直接将数据去除
                CloseFile();
                blen -= boundry_pos;
                memmove(buf, buf + content_pos, blen);
                memset(buf + blen, 0, content_pos);
					      if (GetFileName(buf, &content_pos))
					      {
					      	  CreateFile();
					      	  //将内容以及middle分隔符头部进行删除
					      	  blen -= content_pos;
					      	  //匹配到了就把内容和middle分隔符去除
					      	  memmove(buf, buf + content_pos, blen);
					      	  memset(buf + blen, 0, content_pos);
					      }
					      else 
					      {
					      	  //此时遇到的这个middle分隔符，后面的数据不是为了上传文件
					      	  //头信息不全跳出循环,没找到\r\n\r\n，等待再次从缓存区中拿取数据，再次循环进来进行判断
					      	  if (content_pos == 0)
					      	  {
					      	  	break;
					      	  }
					      	  //没有找到名字或者名字后面的"
					      	  //没有匹配成功就把boundary去除,防止下次进入再找这一个boundary
					      	  blen -= _middle_boundry.length();
					      	  memmove(buf, buf + _middle_boundry.length(), blen);
					      	  memset(buf + blen, 0, _middle_boundry.length());
					      }
            }
            flag = MatchBoundey(buf, blen, &boundry_pos);
            if(flag == BOUNDRY_LAST)
            {
                //last_boundry匹配成功
                //将boundry前的数据写入文件
                //关闭文件
                //上传文件处理完毕，退出
                WriteFile(buf, boundry_pos);
                CloseFile();
                return true;
            }
            flag = MatchBoundey(buf, blen, &boundry_pos);
						if (flag == BOUNDRY_PART)
						{
								//1.将类似boundary位置之前的数据写入文件
								//2.移除之前的数据
								//3.剩下的数据不动，重新继续接收数据，补全后匹配
								WriteFile(buf, boundry_pos);
								blen -= boundry_pos;
								memmove(buf, buf + boundry_pos, blen);
								memset(buf + blen, 0, boundry_pos);
						}
						flag = MatchBoundey(buf, blen, &boundry_pos);
						if (flag == BOUNDRY_NO)
						{
								//将所有数据写入文件
								WriteFile(buf, blen);
								blen = 0;
						}

						tlen += len;
        }

        return true;
    }
};

int main()
{
    UpLoad upload;
    std::string rsp_body;

    if(upload.InitUpLoadInfo() == false)
        return 0;
    
    if(upload.ProcessUpLoad() == true)
        rsp_body = "<html><body><h1>SUCCESS</h1></body></html>";
    else 
        rsp_body = "<html><body><h1>FAILED</h1></body></html>";

    std::cout << rsp_body;
    fflush(stdout);
    return 0;
}
