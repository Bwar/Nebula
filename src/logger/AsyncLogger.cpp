/*******************************************************************************
 * Project:  Nebula
 * @file     AsyncLogger.cpp
 * @brief 
 * @author   Bwar
 * @date:    2022-01-29
 * @note
 * Modify history:
 ******************************************************************************/
#include "AsyncLogger.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <thread>
#include <iostream>
#include <unistd.h>

namespace neb
{

AsyncLogger* AsyncLogger::s_pInstance = nullptr;

AsyncLogger::AsyncLogger(uint32 uiLogFileNum, int iLogLev,
        unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex)
    : m_iLogLevel(iLogLev), m_uiMaxFileSize(uiMaxFileSize),
      m_uiMaxRollFileIndex(uiMaxRollFileIndex),
      m_uiWrittingIndex(0), m_uiLastLogSize(0)
{
    m_vecTimeSec.resize(uiLogFileNum, 0);
    m_vecLogFileName.resize(uiLogFileNum, "");
    m_vecLogFout.resize(uiLogFileNum, nullptr);
    m_vecLogFormatTime.resize(uiLogFileNum, "");
    m_vecLogContent[0].resize(uiLogFileNum, "");
    m_vecLogContent[1].resize(uiLogFileNum, "");
    char szFormat[32] = {0};
    for (uint32 i = 0; i < 1000; ++i)
    {
        snprintf(szFormat, 32, "%u]", i);
        m_vecLogFormatMs.emplace_back(std::move(std::string(szFormat)));
    }
    for (uint32 i = 0; i < 15000; ++i) // a single code file lines should not be larger than 15000
    {
        snprintf(szFormat, 32, ":%u ", i);
        m_vecLogFormatCodeLine.emplace_back(std::move(std::string(szFormat)));
    }
    std::thread t(&AsyncLogger::LogToFile, this);
    t.detach();
}

AsyncLogger::~AsyncLogger()
{
}

AsyncLogger* AsyncLogger::Instance(uint32 uiLogFileNum, int iLogLev,
            unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex)
{
    if (s_pInstance == nullptr)
    {
        s_pInstance = new AsyncLogger(uiLogFileNum, iLogLev, uiMaxFileSize, uiMaxRollFileIndex);
    }
    return(s_pInstance);
}

int AsyncLogger::WriteLog(int iWorkerIndex, int iLev, const char* szFileName,
        unsigned int uiFileLine, const char* szFunction, const std::string& strContent)
{
    if (iLev > m_iLogLevel)
    {
        return(0);
    }
    uint32 uiContentIndex = 0;
    if (iWorkerIndex < 0 || iWorkerIndex >= (int)m_vecLogFout.size())
    {
        uiContentIndex = m_vecLogFout.size() - 1;
    }
    else
    {
        uiContentIndex = (uint32)iWorkerIndex;
    }
    AppendLogPattern(uiContentIndex, iLev, szFileName, uiFileLine, szFunction);
    m_vecLogContent[m_uiWrittingIndex][uiContentIndex].append(strContent);
    m_vecLogContent[m_uiWrittingIndex][uiContentIndex].append("\n");
    return(0);
}

int AsyncLogger::WriteLog(int iWorkerIndex, const std::string& strTraceId, int iLev, const char* szFileName,
        unsigned int uiFileLine, const char* szFunction, const std::string& strContent)
{
    if (iLev > m_iLogLevel)
    {
        return(0);
    }
    uint32 uiContentIndex = 0;
    if (iWorkerIndex < 0 || iWorkerIndex >= (int)m_vecLogFout.size())
    {
        uiContentIndex = m_vecLogFout.size() - 1;
    }
    else
    {
        uiContentIndex = (uint32)iWorkerIndex;
    }
    AppendLogPattern(uiContentIndex, strTraceId, iLev, szFileName, uiFileLine, szFunction);
    m_vecLogContent[m_uiWrittingIndex][uiContentIndex].append(strContent);
    m_vecLogContent[m_uiWrittingIndex][uiContentIndex].append("\n");
    return(0);
}

void AsyncLogger::AddLogFile(int iWorkerIndex, const std::string& strLogFile)
{
    uint32 uiContentIndex = 0;
    if (iWorkerIndex < 0 || iWorkerIndex >= (int)m_vecLogFileName.size())
    {
        uiContentIndex = m_vecLogFileName.size() - 1;
    }
    else
    {
        uiContentIndex = (uint32)iWorkerIndex;
    }
    m_vecLogFileName[uiContentIndex] = strLogFile;
}

void AsyncLogger::AppendLogPattern(uint32 uiIndex, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction)
{
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    if (timeval.tv_sec > m_vecTimeSec[uiIndex])
    {
        char szTime[32];
        m_vecTimeSec[uiIndex] = (time_t)timeval.tv_sec;
        strftime(szTime, 32, "[%Y-%m-%d %H:%M:%S.", std::localtime(&timeval.tv_sec));
        m_vecLogFormatTime[uiIndex] = szTime;
    }
    auto& strLogBuff = m_vecLogContent[m_uiWrittingIndex][uiIndex];
    strLogBuff.append(m_vecLogFormatTime[uiIndex]);
    strLogBuff.append(m_vecLogFormatMs[timeval.tv_usec / 1000]);
    strLogBuff.append(LogLevMsg[iLev]);
    strLogBuff.append(szFileName);
    strLogBuff.append(m_vecLogFormatCodeLine[uiFileLine]);
    strLogBuff.append(szFunction);
    strLogBuff.append(" ");
}

void AsyncLogger::AppendLogPattern(uint32 uiIndex, const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction)
{
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    if (timeval.tv_sec > m_vecTimeSec[uiIndex])
    {
        char szTime[32];
        m_vecTimeSec[uiIndex] = (time_t)timeval.tv_sec;
        strftime(szTime, 32, "[%Y-%m-%d %H:%M:%S.", std::localtime(&timeval.tv_sec));
        m_vecLogFormatTime[uiIndex] = szTime;
    }
    auto& strLogBuff = m_vecLogContent[m_uiWrittingIndex][uiIndex];
    strLogBuff.append(m_vecLogFormatTime[uiIndex]);
    strLogBuff.append(m_vecLogFormatMs[timeval.tv_usec / 1000]);
    strLogBuff.append(LogLevMsg[iLev]);
    strLogBuff.append(szFileName);
    strLogBuff.append(m_vecLogFormatCodeLine[uiFileLine]);
    strLogBuff.append(szFunction);
    strLogBuff.append(" ");
    strLogBuff.append(strTraceId);
    strLogBuff.append(" ");
}

void AsyncLogger::LogToFile()
{
    char szLogCost[200] = {0};
    uint32 uiReadIndex = 0;
    auto time_now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(time_now);
    auto time_from_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
    auto time_to_ms = time_from_ms;
    int64 lCostTimeMs = 0;
    std::ostringstream oss;
    while(true)
    {
        if (m_uiLastLogSize < 1048576)  // When the log volume of a single write is less than 1MB
        {
            sleep(1);
        }
        uiReadIndex = m_uiWrittingIndex;
        m_uiWrittingIndex = 1 - uiReadIndex;
        m_uiLastLogSize = 0;
        uint32 uiLogSize = 0;
        time_now = std::chrono::system_clock::now();
        time_from_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
        for (uint32 i = 0; i < m_vecLogFout.size(); ++i)
        {
            if (m_vecLogFileName[i].size() == 0)
            {
                continue;
            }
            if (m_vecLogFout[i] == nullptr)
            {
                m_vecLogFout[i] = std::make_shared<std::ofstream>();
                if (m_vecLogFout[i] == nullptr)
                {
                    continue;
                }
                m_vecLogFout[i]->open(m_vecLogFileName[i].c_str(), std::ios::app);
            }
            if (m_vecLogContent[uiReadIndex][i].empty())
            {
                continue;
            }
            RollOver(m_vecLogFileName[i], m_vecLogFout[i]);
            if (!m_vecLogFout[i]->good())
            {
                std::cerr << "Can not open file: " << m_vecLogFileName[i] << std::endl;
                continue;
            }
            uiLogSize = m_vecLogContent[uiReadIndex][i].size();
            m_vecLogFout[i]->write(m_vecLogContent[uiReadIndex][i].data(), uiLogSize);
            m_vecLogFout[i]->flush();
            m_vecLogContent[uiReadIndex][i].clear();
            m_uiLastLogSize += uiLogSize;
        }
        time_now = std::chrono::system_clock::now();
        time_to_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
        lCostTimeMs = time_to_ms.count() - time_from_ms.count();
        t = std::chrono::system_clock::to_time_t(time_now);
        oss.str("");
        // There is a bug: The std::get_time and std::put_time manipulators are still missing in gcc 4.9.
        // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54354
        #if __GNUC__ >= 5
            oss << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "."
                << time_to_ms.count() % 1000 << "]" << LogLevMsg[Logger::INFO] << " "
                << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " ";
        #else
            char szTime[32];
            strftime(szTime, 32, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            oss << "[" << szTime << "."
                << time_to_ms.count() % 1000 << "]" << LogLevMsg[Logger::INFO] << " "
                << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << " ";
        #endif
        snprintf(szLogCost, 200, "%s it takes %ldms to write %u logs to log file.\n", oss.str().c_str(), lCostTimeMs, m_uiLastLogSize);
        m_vecLogFout[m_vecLogFout.size() - 1]->write(szLogCost, strlen(szLogCost));
        m_vecLogFout[m_vecLogFout.size() - 1]->flush();
    }
}

void AsyncLogger::RollOver(const std::string& strFileBase, std::shared_ptr<std::ofstream> pFout)
{
    long lFileSize = -1;
    lFileSize = pFout->tellp();
    if (lFileSize < 0)
    {
        pFout->open(strFileBase.c_str(), std::ios::app);
    }
    else if (lFileSize >= m_uiMaxFileSize)
    {
        pFout->close();
        std::stringstream ssOldestFile(
                std::stringstream::in | std::stringstream::out);
        ssOldestFile << strFileBase << "." << m_uiMaxRollFileIndex;
        remove(ssOldestFile.str().c_str());

        for (int i = m_uiMaxRollFileIndex - 1; i >= 1; --i)
        {
            std::stringstream ssSrcFileName(
                    std::stringstream::in | std::stringstream::out);
            std::stringstream ssDestFileName(
                    std::stringstream::in | std::stringstream::out);

            ssSrcFileName << strFileBase << "." << i;
            ssDestFileName << strFileBase << "." << (i + 1);
            remove(ssDestFileName.str().c_str());
            rename(ssSrcFileName.str().c_str(), ssDestFileName.str().c_str());
        }
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << strFileBase << ".1";
        std::string strBackupFile = ss.str();
        rename(strFileBase.c_str(), strBackupFile.c_str());
        pFout->open(strFileBase.c_str(), std::ios::app);
    }
}

} /* namespace neb */

