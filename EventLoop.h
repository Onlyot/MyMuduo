#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// Channel and Poller
class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // start the loop of event
    void loop();
    // quit the loop of event
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}
    
    // exec cb in current loop
    void runInLoop(Functor cb);
    // put cb on queue, weakup the thread of loop
    void queueInLoop(Functor cb);

    // use to weakup the thread of loop
    void wakeup();

    // EventLoop => Poller
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // judge EventLoop whether in thread on that own
    bool isInLoopThread() const {return  threadId_ == CurrentThread::tid();}

private:

    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_; // the sign of quit
    const pid_t threadId_;  // record the thread of currnet loop 
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;
    
    // mainReactor send acceptor to subReactor
    // to handle channel
    int wakeupFd_;
    std::unique_ptr<Channel> weakupChannel_;

    ChannelList activeChannels_;;
    //Channel *currentActiveChannel_;

    // Identify whether the current loop has a callback function that needs to be executed
    std::atomic_bool callingPendingFunctors_;
    std::vector<Functor> pendingFunctors_;  // store the callback that need to be called
    // used to protect the pendingFunctors_
    std::mutex mutex_;
};