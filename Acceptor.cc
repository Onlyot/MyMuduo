#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | O_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__,
                                            __FUNCTION__,
                                            __LINE__, 
                                            errno);
    }
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      accpetChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    accpetChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    accpetChannel_.disableAll();
    accpetChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    listenning_ = true;
    // Socket封装了监听的fd，实现了相应的控制
    acceptSocket_.listen();
    accpetChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr(0, "127.0.0.1");
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__,__FUNCTION__,__LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d socket reached limit! \n", __FILE__,__FUNCTION__,__LINE__);
        }
    }
}