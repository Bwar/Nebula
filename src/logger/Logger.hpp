/*******************************************************************************
 * Project:  Nebula
 * @file     Logger.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 29, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LOGGER_LOGGER_HPP_
#define LOGGER_LOGGER_HPP_

#include <string>

namespace neb
{


const unsigned int gc_uiMaxLogLineLen = 2048;
const unsigned int gc_uiMaxLogFileSize = 2048000;
const unsigned int gc_uiMaxRollLogFileIndex = 9;

class Logger
{
public:
    enum LogLev
    {
        FATAL = 0,        //致命错误
        CRITICAL = 1,     //严重错误
        ERROR = 2,        //一般错误
        NOTICE = 3,       //关键提示消息
        WARNING = 4,      //警告
        INFO = 5,         //一般提示消息
        DEBUG = 6,        //调试消息
        TRACE = 7,        //追踪消息
        LEV_MAX           //日志错误级别数
    };

public:
    Logger(){};
    virtual ~Logger(){};
    virtual int WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* chLogStr = "info", ...) = 0;
    virtual void SetLogLevel(int iLev) = 0;
};

const std::string LogLevMsg[Logger::LEV_MAX] =
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

} /* namespace neb */

#endif /* LOGGER_LOGGER_HPP_ */
