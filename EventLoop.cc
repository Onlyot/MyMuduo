#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

__thread EventLoop *t_loopInThisThread = nullptr;

constexpr int kPollTimeMs = 10000;

// create weakupfd, use to weakup subReactor
// to use new connect
int createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", evtfd);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventFd()),
      weakupChannel_(new Channel(this, wakeupFd_))
      //currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", this, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //Set the event type of Weakupfd 
    // and the callback operation after the event occurs
    weakupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // every EventLoop can receive the message of weakup
    weakupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    weakupChannel_->disableAll();
    weakupChannel_->remove();
    ::close(wakeupFd_);
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    while(!quit_)
    {
        activeChannels_.clear();
        // listen two kinds of fd, client and weakupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(auto channel : activeChannels_)
        {
            // poller -> EventPoller -> handleEvent
            channel->handleEvent(pollReturnTime_);
        }
        // 执行current EventLoop need to do callback
        // subEventLoop 被 mainLoop 唤醒后 需要执行回调函数
        // 下面的回调函数是由 mainLoop指定的
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
}

// quit函数可能被其他线程所调用，那么需要唤醒其所在的线程  
void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

// exec cb in current loop
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else    // 在非当前线程执行，则唤醒loop所在线程执行 cb
    {
        queueInLoop(std::move(cb));
    }
}
// put cb on queue, weakup the thread of loop
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // callingPendingFunctors_ == true 表示loop正在执行回调函数，执行完后要再次唤醒
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

// use to weakup the thread of loop
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(sizeof(one) != n)
    {
        LOG_ERROR("EventLoop::wakeup() writes write %lu bytes instead of 8\n", n);
    }
}

// EventLoop => Poller
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    return poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor(); // 执行当前 loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(sizeof(one) != n)
    {
        LOG_ERROR("EventLoop::handleRead() write %ld bytes instead of 8\n", n);
    }
}
