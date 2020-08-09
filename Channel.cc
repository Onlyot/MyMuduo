#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}

Channel::~Channel()
{

}

// when tie had been call ?
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// when change the events, call the poller(epoll_ctl)
void Channel::update()
{
    loop_->updateChannel(this);
}

// remove channel(as same that remove the fd)
void Channel::remove()
{
    loop_->removeChannel(this);
}
void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if(tied_)
    {
        guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if(revents_ & (EPOLLOUT))
    {
        if(writeCallback_) writeCallback_();
    }
}