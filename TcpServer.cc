#include "TcpServer.h"
#include "TcpConnection.h"
#include "Logger.h"
 
#include <functional>
#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(!loop)
    {
        LOG_FATAL("%s:%s%d mainLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
            const InetAddress &listenAddr, 
            const std::string nameArg,
            Option option)
            : loop_(CheckLoopNotNull(loop)),
              ipPort_(listenAddr.toIpPort()),
              name_(nameArg),
              acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
              threadPool_(new EventLoopThreadPool(loop, nameArg)),
              connectionCallback_(),
              messageCallback_(),
              nextConnId_(1),
              started_(0)
{
    // while new user connecting, do it
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                        std::placeholders::_1, std::placeholders::_2));
    
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    if(started_++ == 0) // 防止TcpServer::start()函数被多次调用
    {
        // 启动底层的线程池
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }

}

// 有一个新的客户端的连接，acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法选择subLoop接管新连接
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
                name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过socket获取其绑定的本地的ip地址和端口信息
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (struct sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 创建连接成功的 sockfd, TcpConnection对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer -> TcpConnection -> channel
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_)                               ;
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this,
                            std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));


}


void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConntionInLoop, this, conn));
}
void TcpServer::removeConntionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConntionInLoop [%s] - connection %s", 
                        name_.c_str(), conn->name().c_str());
    size_t n = connections_.erase(conn->name());

    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

TcpServer::~TcpServer()
{
    LOG_INFO("TcpServer::~TcpServer [%s] destructing", name_.c_str());
    for(auto& item : connections_)
    {
        // 栈上对象可自动释放
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}
