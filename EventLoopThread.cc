#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                            const std::string& name)
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

// 单独开辟一个新线程
EventLoop* EventLoopThread::startLoop()
{
    thread_.start();

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == NULL){
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 这是上面创建的新线程所执行的线程函数
void EventLoopThread::threadFunc()
{
    // one loop per thread
    EventLoop loop;
    if(callback_){
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = NULL;
}