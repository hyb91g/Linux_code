#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__ 

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <stdlib.h>

using namespace std;

typedef int (*cal_t)(int, int);

// + - * /
class Task{
private:
   int _x;
   int _y;
   cal_t _handlerTask;
   int _z;
public:
   Task(int x, int y, cal_t handlerTask)
     :_x(x), _y(y), _handlerTask(handlerTask)
   {}


   void Run()
   {
     _z = _handlerTask(_x, _y);
   }

   void Show()
   {
      cout << "thread " << pthread_self() << " Handler Task Done! " << "Result is : " << _z << endl;
   }

};


class ThreadPool
{
public: 
  ThreadPool(int num):thread_nums(num), is_stop(false)
  {}

  void InitThreadPool()
  {
    pthread_mutex_init(&_lock, NULL);
    pthread_cond_init(&_cond, NULL);
    int i = 0;
    for(; i < thread_nums; i++)
    {
      pthread_t tid;
      pthread_create(&tid, NULL, thread_routine, (void *)this);
    }
  }
  void LockQueue()
  {
    pthread_mutex_lock(&_lock);
  }
  void UnLockQueue()
  {
    pthread_mutex_unlock(&_lock);
  }
  bool isEmpty()
  {
    return t_queue.size() == 0 ? true : false;
  }
  void IdleThread()
  {
    if(is_stop){
      UnLockQueue();
      thread_nums--;
      pthread_exit((void *)0);
      cout << "thread " << pthread_self() << " exit!" << endl;
      return;
    }
    pthread_cond_wait(&_cond, &_lock);
  }
  void AddTask(Task &val)
  {
    LockQueue();
    if(is_stop)
    {
      UnLockQueue();
      return;
    }
    t_queue.push(val);
    NotifyOneThread();
    UnLockQueue();
  }
  Task GetTask()
  {
    Task t = t_queue.front();
    t_queue.pop();
    return t;
  }
  void Stop()
  {
    LockQueue();
    is_stop = true;
    UnLockQueue();
    while(thread_nums > 0){
      NotifyAllThread();
    }
  }
  ~ThreadPool()
  {
    pthread_mutex_destroy(&_lock);
    pthread_cond_destroy(&_cond);
  }

private:
  static void *thread_routine(void *arg)  //static
  {
    pthread_detach(pthread_self());
    ThreadPool* tp = (ThreadPool*) arg;
    for(;;){
      tp->LockQueue();
      while(tp->isEmpty()){
        tp->IdleThread();
      }
      
      Task t = tp->GetTask();  
      
      tp->UnLockQueue();

      //do task 
      t.Run();
      t.Show();
    }
  }
  void NotifyOneThread()
  {
    pthread_cond_signal(&_cond);
  }
  void NotifyAllThread()
  {
    pthread_cond_broadcast(&_cond);
  }

private:
  int thread_nums;
  queue<Task> t_queue;

  pthread_mutex_t _lock;
  pthread_cond_t _cond;
  
  bool is_stop;
};

#endif 
