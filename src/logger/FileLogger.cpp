/*******************************************************************************
 * Project:  Nebula
 * @file     FileLogger.cpp
 * @brief 
 * @author   bwar
 * @date:    Mar 29, 2018
 * @note
 * Modify history:
 ******************************************************************************/

#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <iomanip>
#include <string>
#include <iostream>
#include <fstream>

#include "FileLogger.hpp"

namespace neb
{

FileLogger* FileLogger::s_pInstance = nullptr;

FileLogger::FileLogger(const std::string& strLogFile, int iLogLev,
        unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex, bool bAlwaysFlush)
    : m_iLogLevel(iLogLev), m_uiLogNum(0), m_uiMaxFileSize(uiMaxFileSize),
      m_uiMaxRollFileIndex(uiMaxRollFileIndex), m_bAlwaysFlush(bAlwaysFlush),
      m_lTimeSec(0), m_strLogFileBase(strLogFile)
{
#if __GNUC__ < 5
    m_szTime = (char*)malloc(20);
#endif
    OpenLogFile(strLogFile);
    WriteLog(Logger::NOTICE, __FILE__, __LINE__, __FUNCTION__, "new log instance.");
}

FileLogger::~FileLogger()
{
#if __GNUC__ < 5
    free(m_szTime);
#endif
    m_fout.close();
}

int FileLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, const std::string& strContent)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        ReOpen();
    }
    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(iLev, szFileName, uiFileLine, szFunction);
    Append(strContent);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

int FileLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, const std::string& strContent)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(!m_fout.good())
    {
        ReOpen();
    }
    if(!m_fout.good())
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    AppendLogPattern(strTraceId, iLev, szFileName, uiFileLine, szFunction);
    Append(strContent);
    m_fout << "\n";

    if (m_bAlwaysFlush)
    {
        m_fout.flush();
    }
    ++m_uiLogNum;

    return 0;
}

bool FileLogger::OpenLogFile(const std::string strLogFile)
{
    m_fout.open(strLogFile.c_str(), std::ios::app);
    if(!m_fout.good())
    {
        std::cerr << "Can not open file: " << strLogFile << std::endl;
        return(false);
    }
    return(true);
}

void FileLogger::ReOpen()
{
    m_fout.close();
    m_fout.open(m_strLogFileBase.c_str(), std::ios::app);
}

void FileLogger::RollOver()
{
    if (m_fout.good())
    {
        m_fout.close();
    }
    std::stringstream ssOldestFile(
            std::stringstream::in | std::stringstream::out);
    ssOldestFile << m_strLogFileBase << "." << m_uiMaxRollFileIndex;
    remove(ssOldestFile.str().c_str());

    for (int i = m_uiMaxRollFileIndex - 1; i >= 1; --i)
    {
        std::stringstream ssSrcFileName(
                std::stringstream::in | std::stringstream::out);
        std::stringstream ssDestFileName(
                std::stringstream::in | std::stringstream::out);

        ssSrcFileName << m_strLogFileBase << "." << i;
        ssDestFileName << m_strLogFileBase << "." << (i + 1);
        remove(ssDestFileName.str().c_str());
        rename(ssSrcFileName.str().c_str(), ssDestFileName.str().c_str());
    }
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << m_strLogFileBase << ".1";
    std::string strBackupFile = ss.str();
    rename(m_strLogFileBase.c_str(), strBackupFile.c_str());
}

void FileLogger::AppendLogPattern(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction)
{
    if (m_uiLogNum % 10000 == 1)
    {
        long lFileSize = -1;
        lFileSize = m_fout.tellp();
        if (lFileSize < 0)
        {
            ReOpen();
        }
        else if (lFileSize >= m_uiMaxFileSize)
        {
            RollOver();
            ReOpen();
        }
        if (!m_fout.good())
        {
            return;
        }
    }
    auto time_now = std::chrono::system_clock::now();
    auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
    auto t = std::chrono::system_clock::to_time_t(time_now);
    std::ostringstream oss;
// There is a bug: The std::get_time and std::put_time manipulators are still missing in gcc 4.9.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54354
#if __GNUC__ >= 5
    oss << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "."
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "] ";
#else
    strftime(m_szTime, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    oss << "[" << m_szTime << "."
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "] ";
#endif
    Append(oss.str());
}

void FileLogger::AppendLogPattern(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction)
{
    if (m_uiLogNum % 10000 == 1)
    {
        long lFileSize = -1;
        lFileSize = m_fout.tellp();
        if (lFileSize < 0)
        {
            ReOpen();
        }
        else if (lFileSize >= m_uiMaxFileSize)
        {
            RollOver();
            ReOpen();
        }
        if (!m_fout.good())
        {
            return;
        }
    }
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    if (timeval.tv_sec > m_lTimeSec)
    {
        char szTime[32];
        m_lTimeSec = (time_t)timeval.tv_sec;
        strftime(szTime, 32, "[%Y-%m-%d %H:%M:%S.", std::localtime(&m_lTimeSec));
        m_strLogFormatTime = szTime;
    }
    m_strLogLine.clear();
    m_strLogLine.append(m_strLogFormatTime);
    m_strLogLine.append(std::to_string(timeval.tv_usec / 1000));
    m_strLogLine.append("]");
    m_strLogLine.append(LogLevMsg[iLev]);
    m_strLogLine.append(szFileName);
    m_strLogLine.append(":");
    m_strLogLine.append(std::to_string(uiFileLine));
    m_strLogLine.append(" ");
    m_strLogLine.append(szFunction);
    m_strLogLine.append(" ");
    Append(m_strLogLine);
}

} /* namespace neb */

