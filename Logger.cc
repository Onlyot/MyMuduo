#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// the singleton
Logger& Logger::instance()
{
    static Logger Logger;

    return Logger;
}
// set the level of log
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
// write log
// the format [level] time : msg
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    // print time and msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}