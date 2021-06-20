/*******************************************************************************
 * Project:  Nebula
 * @file     FileLogger.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 29, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LOGGER_FILELOGGER_HPP_
#define LOGGER_FILELOGGER_HPP_

#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Logger.hpp"

namespace neb
{

class FileLogger: public Logger
{
public:
    explicit FileLogger(
            const std::string& strLogFile,
            int iLogLev = Logger::INFO,
            unsigned int uiMaxFileSize = neb::gc_uiMaxLogFileSize,
            unsigned int uiMaxRollFileIndex = neb::gc_uiMaxRollLogFileIndex,
            bool bAlwaysFlush = true);
    virtual ~FileLogger();

    static FileLogger* Instance(const std::string& strLogFile = "../log/default.log",
                    int iLogLev = Logger::INFO,
                    unsigned int uiMaxFileSize = neb::gc_uiMaxLogFileSize,
                    unsigned int uiMaxRollFileIndex = neb::gc_uiMaxRollLogFileIndex)
    {
        if (s_pInstance == nullptr)
        {
            s_pInstance = new FileLogger(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex);
        }
        return(s_pInstance);
    }

    void SetLogLevel(int iLev)
    {
        m_iLogLevel = iLev;
    }

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
    int WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const std::string& strContent);
    int WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, const std::string& strContent);

protected:
    bool OpenLogFile(const std::string strLogFile);
    void ReOpen();
    void RollOver();
    void AppendLogPattern(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction);
    void AppendLogPattern(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction);
    void Append(){}
    template<typename T, typename ...Targs> void Append(T&& arg, Targs&&... args);
    template<typename T> void Append(T&& arg);
    template<typename T, typename ...Targs> void Append(const char* szFormat, T&& arg, Targs&&... args);
    template<typename T> const char* PrintfAppend(const char* szFormat, T&& arg);

private:
    static FileLogger* s_pInstance;
#if __GNUC__ < 5
    char* m_szTime;
#endif
    std::ofstream m_fout;
    int m_iLogLevel;
    unsigned int m_uiLogNum;
    unsigned int m_uiMaxFileSize;       // 日志文件大小
    unsigned int m_uiMaxRollFileIndex;  // 滚动日志文件数量
    bool m_bAlwaysFlush;
    std::string m_strLogFileBase;       // 日志文件基本名（如 log/program_name.log）
};

template<typename ...Targs>
int FileLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, const char* szLogFormat, Targs&&... args)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(iLev, szFileName, uiFileLine, szFunction);
    Append(szLogFormat, std::forward<Targs>(args)...);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

template<typename ...Targs>
int FileLogger::WriteLog(const std::string& strTraceId, int iLev,
        const char* szFileName, unsigned int uiFileLine, const char* szFunction,
        const char* szLogFormat, Targs&&... args)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(strTraceId, iLev, szFileName, uiFileLine, szFunction);
    Append(szLogFormat, std::forward<Targs>(args)...);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

template<typename ...Targs>
int FileLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, Targs&&... args)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(iLev, szFileName, uiFileLine, szFunction);
    Append(std::forward<Targs>(args)...);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

template<typename ...Targs>
int FileLogger::WriteLog(const std::string& strTraceId, int iLev,
        const char* szFileName, unsigned int uiFileLine, const char* szFunction,
        Targs&&... args)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(strTraceId, iLev, szFileName, uiFileLine, szFunction);
    Append(std::forward<Targs>(args)...);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

template<typename T, typename ...Targs>
void FileLogger::Append(T&& arg, Targs&&... args)
{
    Append(std::forward<T>(arg));
    Append(std::forward<Targs>(args)...);
}

template<typename T>
void FileLogger::Append(T&& arg)
{
    m_fout << arg;
}

template<typename T, typename ...Targs>
void FileLogger::Append(const char* szFormat, T&& arg, Targs&&... args)
{
    if (szFormat == nullptr)
    {
        return;
    }
    const char* pFormat = szFormat;
    pFormat = PrintfAppend(pFormat, arg);
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
const char* FileLogger::PrintfAppend(const char* szFormat, T&& arg)
{
    if (szFormat == nullptr)
    {
        return(nullptr);
    }
    const char* pPos = szFormat;
    bool bPlaceholder = false;
    std::string strOutStr;
    int i = 0;
    for (size_t i = 0; ; ++i)
    {
        switch (pPos[i])
        {
            case '\0':
                if (i > 0)
                {
                    strOutStr.assign(szFormat, i);
                    m_fout << strOutStr;
                }
                m_fout << arg;
                return(nullptr);
            case '%':
                if (bPlaceholder)
                {
                    strOutStr.append("%");
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    return(PrintfAppend(pPos, std::forward<T>(arg)));
                }
                else
                {
                    strOutStr.assign(szFormat, i);
                    bPlaceholder = true;
                }
                break;
            case 'a':
            case 'A':
            case 'c':
            case 'C':
            case 'f':
            case 'p':
            case 's':
            case 'S':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << arg;
                    return(pPos);
                }
                break;
            case 'u':
            case 'd':
            case 'i':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::dec << arg;
                    return(pPos);
                }
                break;
            case 'x':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::hex << std::nouppercase << arg;
                    return(pPos);
                }
                break;
            case 'X':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::hex << std::uppercase << arg;
                    return(pPos);
                }
                break;
            case 'o':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::oct << arg;
                    return(pPos);
                }
                break;
            case 'e':
            case 'g':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::scientific << std::nouppercase << arg;
                    return(pPos);
                }
                break;
            case 'E':
            case 'G':
                if (bPlaceholder)
                {
                    pPos = szFormat + i + 1;
                    m_fout << strOutStr;
                    m_fout << std::scientific << std::uppercase << arg;
                    return(pPos);
                }
                break;
            default:
                ;
        }
    }
    strOutStr.assign(szFormat, i);
    m_fout << strOutStr;
    return(nullptr);
}

} /* namespace neb */

#endif
