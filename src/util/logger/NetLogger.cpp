/*******************************************************************************
 * Project:  Nebula
 * @file     NetLogger.cpp
 * @brief 
 * @author   bwar
 * @date:    Mar 30, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include <cstdio>
#include <cstdarg>
#include "FileLogger.hpp"
#include "NetLogger.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

NetLogger::NetLogger(const std::string strLogFile, int iLogLev, unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex, Labor* pLabor)
    : m_pLog(nullptr), m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_bEnableNetLogger(false), m_pLabor(pLabor)
{
    m_pLog = std::make_unique<neb::FileLogger>(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex);
    //m_pLog = std::unique_ptr<FileLogger>(new FileLogger(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex));
}

NetLogger::~NetLogger()
{
}

int NetLogger::WriteLog(int iLev, const char* szLogStr, ...)
{
    char szLogContent[uiMaxLogLineLen] = {0};
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(szLogContent, uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    m_pLog->WriteLog(iLev, szLogContent);
    if (m_bEnableNetLogger && m_pLabor)
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        oTraceLog.set_node_id(m_pLabor->GetNodeId());
        oTraceLog.set_node_identify(m_pLabor->GetNodeIdentify());
        oTraceLog.set_log_content(szLogContent);
        oMsgBody.set_data(oTraceLog.SerializeAsString());
        oMsgBody.mutable_req_target()->set_route(m_pLabor->GetNodeIdentify());
        m_pLabor->SendOriented("LOGGER", CMD_REQ_LOG_TRACE, 0, oMsgBody);
    }

    return 0;
}

int NetLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szLogStr, ...)
{
    char szLogContent[uiMaxLogLineLen] = {0};
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(szLogContent, uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    if (strTraceId == m_pLabor->GetNodeIdentify())
    {
        m_pLog->WriteLog(iLev, szLogContent);
    }
    else
    {
        m_pLog->WriteLog(iLev, "[", strTraceId.c_str(), "] ", szLogContent);
    }
    if (m_bEnableNetLogger && m_pLabor)
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        oTraceLog.set_node_id(m_pLabor->GetNodeId());
        oTraceLog.set_node_identify(m_pLabor->GetNodeIdentify());
        oTraceLog.set_log_content(szLogContent);
        oMsgBody.set_trace_id(strTraceId);
        oMsgBody.set_data(oTraceLog.SerializeAsString());
        oMsgBody.mutable_req_target()->set_route(strTraceId);
        m_pLabor->SendOriented("LOGGER", CMD_REQ_LOG_TRACE, 0, oMsgBody);
    }

    return 0;
}

} /* namespace neb */
