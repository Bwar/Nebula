/*******************************************************************************
 * Project:  Nebula
 * @file     AsyncLogger.hpp
 * @brief 
 * @author   Bwar
 * @date:    2022-01-29
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LOGGER_ASYNCLOGGER_HPP_
#define SRC_LOGGER_ASYNCLOGGER_HPP_

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <ctime>
#include <sys/time.h>
#include "Definition.hpp"
#include "Logger.hpp"

namespace neb
{

class AsyncLogger
{
public:
    virtual ~AsyncLogger();

    int WriteLog(int iWorkerIndex, int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const std::string& strContent);
    int WriteLog(int iWorkerIndex, const std::string& strTraceId, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction, const std::string& strContent);

    void AddLogFile(int iWorkerIndex, const std::string& strLogFile);

    void SetLogLevel(int iLev)
    {
        m_iLogLevel = iLev;
    }

    static AsyncLogger* Instance(
            uint32 uiLogFileNum,
            int iLogLev = Logger::INFO,
            unsigned int uiMaxFileSize = neb::gc_uiMaxLogFileSize,
            unsigned int uiMaxRollFileIndex = neb::gc_uiMaxRollLogFileIndex);
    static inline AsyncLogger* Instance()
    {
        return(s_pInstance);
    }

protected:
    void AppendLogPattern(uint32 uiIndex, int iLev, const char* szFileName,
            unsigned int uiFileLine, const char* szFunction);
    void AppendLogPattern(uint32 uiIndex, const std::string& strTraceId,
            int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction);
    void LogToFile();
    void RollOver(const std::string& strFileBase, std::shared_ptr<std::ofstream> pFout);

private:
    explicit AsyncLogger(
            uint32 uiLogFileNum,
            int iLogLev = Logger::INFO,
            unsigned int uiMaxFileSize = neb::gc_uiMaxLogFileSize,
            unsigned int uiMaxRollFileIndex = neb::gc_uiMaxRollLogFileIndex);

    static AsyncLogger* s_pInstance;

private:
    int m_iLogLevel;
    uint32 m_uiMaxFileSize;       // 日志文件大小
    uint32 m_uiMaxRollFileIndex;  // 滚动日志文件数量
    uint32 m_uiWrittingIndex;
    uint32 m_uiLastLogSize;
    std::vector<time_t> m_vecTimeSec;
    std::vector<std::string> m_vecLogFileName;
    std::vector<std::shared_ptr<std::ofstream>> m_vecLogFout;
    std::vector<std::string> m_vecLogFormatTime;
    std::vector<std::string> m_vecLogFormatMs;
    std::vector<std::string> m_vecLogFormatCodeLine;
    std::vector<std::string> m_vecLogContent[2];
};

} /* namespace neb */

#endif /* SRC_LOGGER_ASYNCLOGGER_HPP_ */

