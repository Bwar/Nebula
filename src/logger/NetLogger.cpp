/*******************************************************************************
 * Project:  Nebula
 * @file     NetLogger.cpp
 * @brief 
 * @author   bwar
 * @date:    Mar 30, 2018
 * @note
 * Modify history:
 ******************************************************************************/

#include "NetLogger.hpp"
#include <cstdio>
#include <cstdarg>
#include "pb/msg.pb.h"
#include "pb/neb_sys.pb.h"
#include "labor/NodeInfo.hpp"
#include "labor/Labor.hpp"
#include "labor/Worker.hpp"
#include "actor/cmd/CW.hpp"

namespace neb
{

NetLogger::NetLogger(const std::string& strLogFile, int iLogLev, unsigned int uiMaxFileSize,
        unsigned int uiMaxRollFileIndex, bool bAlwaysFlush, bool bConsoleLog, Labor* pLabor)
    : m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_iWorkerIndex(0),
      m_bEnableNetLogger(false), m_pLabor(pLabor), m_pLog(nullptr)
{
#if __cplusplus >= 201401L
    m_pLog = std::make_unique<neb::FileLogger>(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex, bAlwaysFlush, bConsoleLog);
#else
    m_pLog = std::unique_ptr<FileLogger>(new FileLogger(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex, bAlwaysFlush, bConsoleLog));
#endif
}

NetLogger::NetLogger(int iWorkerIndex, const std::string& strLogFile, int iLogLev, unsigned int uiMaxFileSize,
        unsigned int uiMaxRollFileIndex, Labor* pLabor)
    : m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_iWorkerIndex(iWorkerIndex),
      m_bEnableNetLogger(false), m_pLabor(pLabor), m_pLog(nullptr)
{
    AsyncLogger::Instance()->AddLogFile(iWorkerIndex, strLogFile);
}

NetLogger::~NetLogger()
{
}

void NetLogger::WriteFileLog(int iLev, const char* szFileName, unsigned int uiFileLine,
        const char* szFunction, const std::string& strLogContent)
{
    if (m_pLog == nullptr) // m_pLabor->GetNodeInfo().bAsyncLogger
    {
        AsyncLogger::Instance()->WriteLog(m_iWorkerIndex, iLev, szFileName, uiFileLine, szFunction, strLogContent);
    }
    else
    {
        m_pLog->WriteLog(iLev, szFileName, uiFileLine, szFunction, strLogContent);
    }
}

void NetLogger::WriteFileLog(const std::string& strTraceId, int iLev, const char* szFileName,
        unsigned int uiFileLine, const char* szFunction, const std::string& strLogContent)
{
    if (m_pLog == nullptr) // m_pLabor->GetNodeInfo().bAsyncLogger
    {
        AsyncLogger::Instance()->WriteLog(m_iWorkerIndex, strTraceId, iLev, szFileName, uiFileLine, szFunction, strLogContent);
    }
    else
    {
        m_pLog->WriteLog(strTraceId, iLev, szFileName, uiFileLine, szFunction, strLogContent);
    }
}

void NetLogger::SinkLog(int iLev, const char* szFileName, unsigned int uiFileLine,
            const char* szFunction, const std::string& strLogContent, const std::string& strTraceId)
{
    if (m_bEnableNetLogger && m_pLabor)
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        oTraceLog.set_node_type(m_pLabor->GetNodeInfo().strNodeType);
        oTraceLog.set_node_identify(m_pLabor->GetNodeInfo().strNodeIdentify);
        oTraceLog.set_log_level(LogLevMsg[iLev]);
        oTraceLog.set_code_file_name(szFileName);
        oTraceLog.set_code_file_line(uiFileLine);
        oTraceLog.set_code_function(szFunction);
        oTraceLog.set_log_content(strLogContent);
        if (strTraceId.size() > 0)
        {
            oMsgBody.set_trace_id(strTraceId);
            oMsgBody.mutable_req_target()->set_route(strTraceId);
        }
        else
        {
            oMsgBody.mutable_req_target()->set_route(m_pLabor->GetNodeInfo().strNodeIdentify);
        }
        oTraceLog.SerializeToString(&m_strOutStr);
        oMsgBody.set_data(m_strOutStr);
        m_pLabor->AddNetLogMsg(oMsgBody);
    }
}

} /* namespace neb */

