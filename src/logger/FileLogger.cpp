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
#include <sstream>
#include <fstream>

#include "FileLogger.hpp"

namespace neb
{

FileLogger* FileLogger::s_pInstance = nullptr;

FileLogger::FileLogger(const std::string& strLogFile, int iLogLev,
        unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex)
    : m_iLogLevel(iLogLev), m_uiLogNum(0), m_uiMaxFileSize(uiMaxFileSize),
      m_uiMaxRollFileIndex(uiMaxRollFileIndex), m_strLogFileBase(strLogFile)
{
#if __GNUC__ < 5
    m_szTime = (char*)malloc(20);
#endif
    m_fp = NULL;
    OpenLogFile(strLogFile);
    WriteLog(Logger::NOTICE, __FILE__, __LINE__, __FUNCTION__, "new log instance.");
}

int FileLogger::OpenLogFile(const std::string strLogFile)
{
    m_fp = fopen(strLogFile.c_str(), "a+" );
    if(NULL == m_fp)
    {
        std::cerr << "Can not open file: " << strLogFile << std::endl;
        return -1;
    }
    return 0;
}

int FileLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, ...)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(NULL == m_fp)
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    va_list ap;
    va_start(ap, szLogStr);
    Vappend(iLev, szFileName, uiFileLine, szFunction, szLogStr, ap);
    va_end(ap);

    fprintf(m_fp, "\n");

    fflush(m_fp);
    ++m_uiLogNum;

    return 0;
}

int FileLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, ...)
{
    if (iLev > m_iLogLevel)
    {
        return 0;
    }

    if(NULL == m_fp)
    {
        std::cerr << "Write log error: no log file handle." << std::endl;
        return -1;
    }

    va_list ap;
    va_start(ap, szLogStr);
    Vappend(strTraceId, iLev, szFileName, uiFileLine, szFunction, szLogStr, ap);
    va_end(ap);

    fprintf(m_fp, "\n");
    ++m_uiLogNum;

    fflush(m_fp);

    return 0;
}

void FileLogger::ReOpen()
{
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    m_fp = fopen(m_strLogFileBase.c_str(), "a+");
}

void FileLogger::RollOver()
{
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
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

int FileLogger::Vappend(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, va_list ap)
{
    long file_size = -1;
    if (NULL != m_fp)
    {
        file_size = ftell(m_fp);
    }
    //    if (0 == fstat(m_fd, &sb))
    //    {
    //        file_size = sb.st_size;
    //    }
    if (file_size < 0)
    {
        ReOpen();
    }
    else if (file_size >= m_uiMaxFileSize)
    {
        RollOver();
        ReOpen();
    }
    if (NULL == m_fp)
    {
        return -1;
    }
    auto time_now = std::chrono::system_clock::now();
    auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
    auto t = std::chrono::system_clock::to_time_t(time_now);
    std::ostringstream oss;
// There is a bug: The std::get_time and std::put_time manipulators are still missing in gcc 4.9.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54354
#if __GNUC__ >= 5
    oss << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << ","
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "] ";
#else
    strftime(m_szTime, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    oss << "[" << m_szTime << ","
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "] ";
#endif
    fprintf(m_fp, oss.str().c_str());
    //fprintf(m_fp, "[%s] [%s,%03d] ", std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S").c_str(), duration_in_ms.count() % 1000, Logger::LogLevMsg[iLev].c_str());
    vfprintf(m_fp, szLogStr, ap);
    fflush(m_fp);
    return 0;
}

int FileLogger::Vappend(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, va_list ap)
{
    if (m_uiLogNum % 10000 == 1)
    {
        long file_size = -1;
        if (NULL != m_fp)
        {
            file_size = ftell(m_fp);
        }
        //    if (0 == fstat(m_fd, &sb))
        //    {
        //        file_size = sb.st_size;
        //    }
        if (file_size < 0)
        {
            ReOpen();
        }
        else if (file_size >= m_uiMaxFileSize)
        {
            RollOver();
            ReOpen();
        }
    }
    if (NULL == m_fp)
    {
        return -1;
    }
    auto time_now = std::chrono::system_clock::now();
    auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
    auto t = std::chrono::system_clock::to_time_t(time_now);
    std::ostringstream oss;
#if __GNUC__ >= 5
    oss << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << ","
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "][" << strTraceId << "] ";
#else
    strftime(m_szTime, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    oss << "[" << m_szTime << ","
        << duration_in_ms.count() % 1000 << "][" << LogLevMsg[iLev] << "]["
        << szFileName << ":" << uiFileLine << "][" << szFunction << "][" << strTraceId << "] ";
#endif
    fprintf(m_fp, oss.str().c_str());
    //fprintf(m_fp, "[%s] [%s,%03d] ", std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S").c_str(), duration_in_ms.count() % 1000, Logger::LogLevMsg[iLev].c_str());
    vfprintf(m_fp, szLogStr, ap);
    fflush(m_fp);
    return 0;
}

} /* namespace neb */
