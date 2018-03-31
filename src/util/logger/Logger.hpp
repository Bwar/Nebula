/*******************************************************************************
 * Project:  Nebula
 * @file     Logger.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 29, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_UTIL_LOGGER_LOGGER_HPP_
#define SRC_UTIL_LOGGER_LOGGER_HPP_

namespace neb
{


class Logger
{
public:
    enum LogLev
    {
        FATAL = 0,          //致命错误
        CRITICAL = 1,       //严重错误
        ERROR = 2,          //一般错误
        NOTICE = 3,         //关键提示消息
        WARNING = 4,        //警告
        INFO = 5,           //一般提示消息
        DEBUG = 6,          //调试消息
        TRACE = 7,          //追踪消息
        LEV_MAX             //日志错误级别数
    };

    const std::string LogLevMsg[LEV_MAX] =
    {
        "FATAL",            //致命错误
        "CRITICAL",         //严重错误
        "ERROR",            //一般错误
        "NOTICE",           //关键提示消息
        "WARNING",          //警告
        "INFO",             //一般提示消息
        "DEBUG",            //调试消息
        "TRACE"
    };

    const int DEFAULT_LOG_LEVEL = INFO;

    const unsigned int uiMaxLogLineLen = 256;
    const unsigned int uiMaxLogFileSize = 2048000;
    const unsigned int uiMaxRollLogFileIndex = 9;

public:
    Logger(){};
    virtual ~Logger(){};
    virtual int WriteLog(int iLev = INFO, const char* chLogStr = "info", ...) = 0;
    virtual void SetLogLevel(int iLev) = 0;
};

} /* namespace neb */

#endif /* SRC_UTIL_LOGGER_LOGGER_HPP_ */
