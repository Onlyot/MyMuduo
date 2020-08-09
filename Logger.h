#pragma once

#include <string>

#include "noncopyable.h"


// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance();    \
        logger.setLogLevel(INFO);   \
        char buf[1024] = {0,};  \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);    \
    }while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance();    \
        logger.setLogLevel(ERROR);   \
        char buf[1024] = {0,};  \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf);    \
    }while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance();    \
        logger.setLogLevel(FATAL);   \
        char buf[1024] = {0,};  \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf);    \
        exit(-1);   \
    }while(0);

#ifdef MUDEBUG

#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    {   \2
        Logger &logger = Logger::instance();    \
        logger.setLogLevel(DEBUG);   \
        char buf[1024] = {0,};  \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf);    \
    }while(0);

#else 
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// define the level of log: INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,   // normal
    ERROR,  // error
    FATAL,  // core 
    DEBUG,  // debug
};

class Logger :   noncopyable
{
public:
    // the singleton
    static Logger& instance();
    // set the level of log
    void setLogLevel(int level);
    // write log
    void log(std::string msg);
private:
    int logLevel_;
    Logger(){}
};