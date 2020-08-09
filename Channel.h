#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;
class Timestamp;

// Channel is store the socket and the interested (re)event such as epollin,epollout
class Channel : noncopyable{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd receive poller, call the eventcallback
    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb)
    {readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb)
    {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb)
    {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb)
    {errorCallback_ = std::move(cb);}
    
    // when channel had been removed, avoid channel execute callback
    void tie(const std::shared_ptr<void>& obj);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    // change the event on fd
    void enableReading(){events_ |= kReadEvent; update();}
    void disableReading(){events_ &= ~kReadEvent; update();}
    void enableWriting(){events_ |= kWriteEvent; update();}
    void disableWriting(){events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ &= kNoneEvent; update();}

    // return the state of event
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isReading() const {return events_ == kReadEvent;}
    bool isWriting() const {return events_ == kWriteEvent;}

    // use by poller
    int index() {return index_;}
    void set_index(int idx){index_ = idx;}

    // one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    void remove();

private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;    // intersting things
    int revents_;   // had occured
    int index_;

    // avoid the cicle reference
    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};