#include "InetAddress.h"
#include "Logger.h"
#include "Socket.h"


#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    int ret = ::bind(sockfd_, (struct sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in));
    if(0 != ret)
    {
        LOG_FATAL("bind sockfd: %d fail \n", sockfd_);
    }
}
void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if(0 != ret)
    {
        LOG_FATAL("listen sockfd: %d fail \n", sockfd_);
    }
}
int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    //int connfd = ::accept(sockfd_, (struct sockaddr*)&addr, &len);
    int connfd = ::accept4(sockfd_, 
                    (struct sockaddr*)&addr, 
                    &len, 
                    SOCK_CLOEXEC | SOCK_NONBLOCK);
    if(connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }

    return connfd;
}

void Socket::shutdownWrite()
{
    int ret = ::shutdown(sockfd_, SHUT_WR);
    if(ret < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}


void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,&optval, static_cast<socklen_t>(sizeof(optval)));
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
}