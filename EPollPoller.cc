#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// had not been added to epoll
constexpr int kNew = -1;    // channel->index_  -1;
// had been added to epoll
constexpr int kAdded = 1;
// had been deleted from epoll
constexpr int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epoll_fd(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if(epoll_fd < 0){
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epoll_fd);
}


Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // should use log_debug, because of this function been called frequently
    LOG_INFO("func=%s => fd total count: %d\n", __FUNCTION__,static_cast<int>(channels_.size()));
    int numEvents = ::epoll_wait(epoll_fd, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err \n");
        }
    }
    return now;
}


void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // return poller to EventLoop
    }
}

// EventLoop -> Channel::update -> Poller::updateChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__,channel->fd(), channel->events(), index);

    if(kNew == index || kDeleted == index)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            // assert
            channels_[fd] = channel;
        }
        else // kDeleted
        {
            // had been added to map but not added to epoll
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        // not interest anything
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// epoll_ctl
void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(::epoll_ctl(epoll_fd, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}


// delete channel from epoll and channels_
void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__,channel->fd());

    const int index = channel->index();
    if(kAdded == index)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}
