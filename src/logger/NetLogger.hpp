/*******************************************************************************
 * Project:  Nebula
 * @file     NetLogger.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 30, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LOGGER_NETLOGGER_HPP_
#define LOGGER_NETLOGGER_HPP_

#include <cstdio>
#include <memory>
#include "FileLogger.hpp"

namespace neb
{

class Labor;

class NetLogger: public Logger
{
public:
    NetLogger(
        const std::string strLogFile,
        int iLogLev = Logger::INFO,
        unsigned int uiMaxFileSize = gc_uiMaxLogFileSize,
        unsigned int uiMaxRollFileIndex = gc_uiMaxRollLogFileIndex,
        unsigned int uiMaxLogLineLen = gc_uiMaxLogLineLen,
        Labor* pLabor = nullptr);
    virtual ~NetLogger();

    int WriteLog(int iLev,
                    const char* szFileName, unsigned int uiFileLine, const char* szFunction,
                    const char* szLogStr = "info", ...);
    int WriteLog(const std::string& strTraceId, int iLev,
                    const char* szFileName, unsigned int uiFileLine, const char* szFunction,
                    const char* szLogStr = "info", ...);

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
    char* m_pLogBuff;
    int m_iLogLevel;
    int m_iNetLogLevel;
    unsigned int m_uiMaxLogLineLen;
    bool m_bEnableNetLogger;
    Labor* m_pLabor;
    std::string m_strLogData;   ///< 用于提高序列化效率
    std::unique_ptr<neb::FileLogger> m_pLog;
};

} /* namespace neb */

#endif /* LOGGER_NETLOGGER_HPP_ */
