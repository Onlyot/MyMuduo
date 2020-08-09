#include "Thread.h"
#include "Logger.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& n)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(n)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0,};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    thread_.reset(new std::thread([&](){

        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_();
    }));

    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}