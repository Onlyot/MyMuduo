#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, 
            const InetAddress &addr,
            const std::string name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, 
                                this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage,
                                this, std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3));
        
        server_.setThreadNum(3);
        // 设置合适的loop线程数
    }

    void start()
    {
        server_.start();
    }

private:

    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connnection UP : %s ", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connnection DOWN : %s ", conn->peerAddress().toIpPort().c_str());
        }        
    }

    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);

        conn->shutdown();   // epollhup  CloseCallback
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{

    EventLoop loop;
    InetAddress addr(static_cast<uint16_t>(8000), "127.0.0.1");

    EchoServer server(&loop, addr, "EchoServer-01");
    server.start(); // Socket->Acceptor->Channel->Poller
    loop.loop();    // EventLoop->Poller

    return 0;
}