/*******************************************************************************
 * Project:  Nebula
 * @file     NetLogger.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 30, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_UTIL_LOGGER_NETLOGGER_HPP_
#define SRC_UTIL_LOGGER_NETLOGGER_HPP_

#include <cstdio>
#include <memory>
#include "util/logger/Logger.hpp"
#include "actor/Actor.hpp"

namespace neb
{

class Labor;

class NetLogger: public Logger
{
public:
    NetLogger(
        const std::string strLogFile,
        int iLogLev = Logger::DEFAULT_LOG_LEVEL,
        unsigned int uiMaxFileSize = Logger::uiMaxLogFileSize,
        unsigned int uiMaxRollFileIndex = Logger::uiMaxRollLogFileIndex,
        Labor* pLabor = nullptr);
    virtual ~NetLogger();

    int WriteLog(int iLev = INFO, const char* szLogStr = "info", ...);
    int WriteLog(const std::string& strTraceId, int iLev = INFO, const char* szLogStr = "info", ...);

    virtual void SetLogLevel(int iLev)
    {
        m_iLogLevel = iLev;
    }

    virtual void SetNetLogLevel(int iLev)
    {
        m_iNetLogLevel = iLev;
    }

    virtual void EnableNetLogger(bool bEnableNetLogger)
    {
        m_bEnableNetLogger = bEnableNetLogger;
    }

private:
    std::unique_ptr<neb::Logger> m_pLog;
    int m_iLogLevel;
    int m_iNetLogLevel;
    bool m_bEnableNetLogger;
    Labor* m_pLabor;
};

} /* namespace neb */

#endif /* SRC_UTIL_LOGGER_NETLOGGER_HPP_ */
