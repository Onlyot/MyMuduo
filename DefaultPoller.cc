#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>

// Dependency inversion principle
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop);
    }
}