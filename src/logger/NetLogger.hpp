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
#include <sstream>
#include "Logger.hpp"
#include "FileLogger.hpp"
#include "AsyncLogger.hpp"

namespace neb
{

class Labor;

class NetLogger: public Logger
{
public:
    NetLogger(
        const std::string& strLogFile,
        int iLogLev = Logger::INFO,
        unsigned int uiMaxFileSize = gc_uiMaxLogFileSize,
        unsigned int uiMaxRollFileIndex = gc_uiMaxRollLogFileIndex,
        bool bAlwaysFlush = true,
        bool bConsoleLog = false,
        Labor* pLabor = nullptr);
    NetLogger(
        int iWorkerIndex,
        const std::string& strLogFile,
        int iLogLev = Logger::INFO,
        unsigned int uiMaxFileSize = gc_uiMaxLogFileSize,
        unsigned int uiMaxRollFileIndex = gc_uiMaxRollLogFileIndex,
        Labor* pLabor = nullptr);
    virtual ~NetLogger();

    template<typename ...Targs>
    int WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const char* szLogFormat, Targs&&... args);
    template<typename ...Targs>
    int WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, const char* szLogFormat, Targs&&... args);
    template<typename ...Targs>
    int WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, Targs&&... args);
    template<typename ...Targs>
    int WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, Targs&&... args);

    virtual void SetLogLevel(int iLev)
    {
        m_iLogLevel = iLev;
        if (m_pLog == nullptr)
        {
            AsyncLogger::Instance()->SetLogLevel(iLev);
        }
        else
        {
            m_pLog->SetLogLevel(iLev);
        }
    }

    virtual void SetNetLogLevel(int iLev)
    {
        m_iNetLogLevel = iLev;
    }

    virtual void EnableNetLogger(bool bEnableNetLogger)
    {
        m_bEnableNetLogger = bEnableNetLogger;
    }

protected:
    void Append(){}
    template<typename T, typename ...Targs> void Append(T&& arg, Targs&&... args);
    template<typename T> void Append(T&& arg);
    template<typename T, typename ...Targs> void Append(const char* szFormat, T&& arg, Targs&&... args);
    template<typename T> const char* PrintfAppend(const char* szFormat, T&& arg);
    void WriteFileLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const std::string& strLogContent);
    void WriteFileLog(const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, const std::string& strLogContent);
    void SinkLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const std::string& strLogContent, const std::string& strTraceId = "");

private:
    int m_iLogLevel;
    int m_iNetLogLevel;
    int m_iWorkerIndex;
    bool m_bEnableNetLogger;
    std::ostringstream m_ossLogContent;
    Labor* m_pLabor;
    std::string m_strOutStr;
    std::unique_ptr<neb::FileLogger> m_pLog;
};

template<typename ...Targs>
int NetLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, const char* szLogFormat, Targs&&... args)
{
    if (iLev > m_iLogLevel && iLev > m_iNetLogLevel)
    {
        return(0);
    }
    m_ossLogContent.str("");
    Append(szLogFormat, std::forward<Targs>(args)...);

    WriteFileLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    SinkLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());
    return 0;
}

template<typename ...Targs>
int NetLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
        unsigned int uiFileLine, const char* szFunction, const char* szLogFormat, Targs&&... args)
{
    if (iLev > m_iLogLevel && iLev > m_iNetLogLevel)
    {
        return(0);
    }
    m_ossLogContent.str("");
    Append(szLogFormat, std::forward<Targs>(args)...);

    WriteFileLog(strTraceId, iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    SinkLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str(), strTraceId);

    return 0;
}

template<typename ...Targs>
int NetLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, Targs&&... args)
{
    if (iLev > m_iLogLevel && iLev > m_iNetLogLevel)
    {
        return(0);
    }
    m_ossLogContent.str("");
    Append(std::forward<Targs>(args)...);

    WriteFileLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    SinkLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());
    return 0;
}

template<typename ...Targs>
int NetLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
        unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    if (iLev > m_iLogLevel && iLev > m_iNetLogLevel)
    {
        return(0);
    }
    m_ossLogContent.str("");
    Append(std::forward<Targs>(args)...);

    WriteFileLog(strTraceId, iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str());

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    SinkLog(iLev, szFileName, uiFileLine, szFunction, m_ossLogContent.str(), strTraceId);

    return 0;
}

template<typename T, typename ...Targs>
void NetLogger::Append(T&& arg, Targs&&... args)
{
    Append(std::forward<T>(arg));
    Append(std::forward<Targs>(args)...);
}

template<typename T>
void NetLogger::Append(T&& arg)
{
    m_ossLogContent << arg;
}

template<typename T, typename ...Targs>
void NetLogger::Append(const char* szFormat, T&& arg, Targs&&... args)
{
    if (szFormat == nullptr)
    {
        return;
    }
    const char* pFormat = szFormat;
    pFormat = PrintfAppend(pFormat, std::forward<T>(arg));
    if (pFormat == nullptr)
    {
        Append(std::forward<Targs>(args)...);
    }
    else
    {
        Append(pFormat, std::forward<Targs>(args)...);
    }
}

template<typename T>
const char* NetLogger::PrintfAppend(const char* szFormat, T&& arg)
{
    if (szFormat == nullptr)
    {
        return(nullptr);
    }
    const char* pPos = szFormat;
    bool bPlaceholder = false;
    m_strOutStr.clear();
    int i = 0;
    for (size_t i = 0; ; ++i)
    {
        switch (pPos[i])
        {
            case '\0':
                if (i > 0)
                {
                    m_strOutStr.assign(szFormat, i);
                    m_ossLogContent << m_strOutStr;
                }
                m_ossLogContent << arg;
                return(nullptr);
            case '%':
                if (bPlaceholder)
                {
                    m_strOutStr.append("%");
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    return(PrintfAppend(pPos, std::forward<T>(arg)));
                }
                else
                {
                    m_strOutStr.assign(szFormat, i);
                    bPlaceholder = true;
                }
                break;
            case 'f':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::fixed << arg;
                    return(pPos);
                }
                break;
            case 'a':
            case 'A':
            case 'c':
            case 'C':
            case 'p':
            case 's':
            case 'S':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << arg;
                    return(pPos);
                }
                break;
            case 'u':
            case 'd':
            case 'i':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::dec << arg;
                    return(pPos);
                }
                break;
            case 'x':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::hex << std::nouppercase << arg;
                    return(pPos);
                }
                break;
            case 'X':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::hex << std::uppercase << arg;
                    return(pPos);
                }
                break;
            case 'o':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::oct << arg;
                    return(pPos);
                }
                break;
            case 'e':
            case 'g':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::scientific << std::nouppercase << arg;
                    return(pPos);
                }
                break;
            case 'E':
            case 'G':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_ossLogContent << m_strOutStr;
                    m_ossLogContent << std::scientific << std::uppercase << arg;
                    return(pPos);
                }
                break;
            default:
                ;
        }
    }
    m_strOutStr.assign(szFormat, i);
    m_ossLogContent << m_strOutStr;
    return(nullptr);
}

} /* namespace neb */

#endif /* LOGGER_NETLOGGER_HPP_ */
