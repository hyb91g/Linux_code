#pragma once 
#include <iostream>
#include <queue>
#include <pthread.h>
#include <mutex>

typedef bool (*Handler)(int sock);
class HttpTask
{
    //http请求处理的任务
    //包含一个成员就是socket
    //包含一个任务处理接口
private:
    int _cli_sock;
    Handler TaskHandler; 
public:
    HttpTask(int sock, Handler handle):_cli_sock(sock), TaskHandler(handle)
    {}
    void Handle() {
        TaskHandler(_cli_sock);
    }
};

class ThreadPool
{
    //线程池类
    //创建指定数量的线程
    //创建一个线程安全的任务队列
    //提供任务的入队，出队，线程池销毁/初始化接口
    private:
        int _max_thr;//线程池的线程数
        bool _is_stop;
        std::queue<HttpTask> _task_queue;
        pthread_mutex_t _mutex;
        pthread_cond_t  _cond;
    private:
        static void *thr_start(void *arg)//完成线程获取任务，并执行任务
        {
            pthread_detach(pthread_self());

            ThreadPool* tp = (ThreadPool*)arg;
            tp->LockQueue();
            while(1)   //每个线程执行完任务后继续上来等待下一个任务
            {
                while(tp->_task_queue.empty())
                {
                    tp->IdleThread();   //任务队列为空，则等待
                }
                //任务队列不为空
                HttpTask ht = tp->PopTask();
                tp->UnLockQueue();

                ht.Handle();
            }
        }

        bool LockQueue()
        {
            pthread_mutex_lock(&_mutex);
            return true;
        }
        bool UnLockQueue()
        {
            pthread_mutex_unlock(&_mutex);
            return true;
        }

        bool NotifyOneThread()
        {
            pthread_cond_signal(&_cond);
            return true;
        }
        bool NotifyAllThread()
        {
            pthread_cond_broadcast(&_cond);
            return true;
        }
    public:
        ThreadPool(int max):_max_thr(max), _is_stop(false)
        {
            //互斥锁/条件变量初始化
            pthread_mutex_init(&_mutex,NULL);
            pthread_cond_init(&_cond, NULL);
        }

        bool ThreadPoolInit() //完成线程创建
        {
            pthread_t tid;
            for(int i = 0; i < _max_thr; i++)
            {
                pthread_create(&tid, NULL, thr_start, this); 
            }
            return true;
        }

        bool PushTask(HttpTask &tt)//线程安全的任务入队
        {
            LockQueue();
            if(_is_stop)
            {
                UnLockQueue();
                return false;
            }
            _task_queue.push(tt);
            NotifyOneThread();  //唤醒一个线程
            UnLockQueue();
            return true;
        }

        HttpTask PopTask()//线程安全的任务出队
        {
            HttpTask tt = _task_queue.front(); 
            _task_queue.pop();

            return tt;
        }

        bool Stop() //销毁
        {
            LockQueue();
            _is_stop = true;
            UnLockQueue();
            while(_max_thr > 0){   
                NotifyAllThread();
            }
            return true;
        }

        bool IdleThread()
        {
            if(_is_stop)
            {
                LockQueue();
                _max_thr--;
                pthread_exit((void*)0);
                std::cout << "thread exit!" << std::endl;
                return false;
            }
            pthread_cond_wait(&_cond, &_mutex);
            return true;
        }

};
