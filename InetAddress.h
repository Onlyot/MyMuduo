#pragma once

#include <netinet/in.h>
#include <string>

class InetAddress{

public:
    explicit InetAddress(uint16_t port, std::string ip);
    explicit InetAddress(struct sockaddr_in addr)
        : addr_(addr)
        {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    // const struct sockaddr* getSockAddr(){ return reinterpret_cast<sockaddr*>(&addr_);}
    const struct sockaddr_in* getSockAddr() const { return &addr_;}
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    struct sockaddr_in addr_;
};