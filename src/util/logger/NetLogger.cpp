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
#include "pb/msg.pb.h"
#include "pb/neb_sys.pb.h"
#include "labor/Labor.hpp"
#include "actor/cmd/CW.hpp"
#include "NetLogger.hpp"

namespace neb
{

NetLogger::NetLogger(const std::string strLogFile, int iLogLev, unsigned int uiMaxFileSize, unsigned int uiMaxRollFileIndex, Labor* pLabor)
    : m_pLog(nullptr), m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_bEnableNetLogger(false), m_pLabor(pLabor)
{
#if __cplusplus >= 201401L
    m_pLog = std::make_unique<neb::FileLogger>(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex);
#else
    m_pLog = std::unique_ptr<FileLogger>(new FileLogger(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex));
#endif
}

NetLogger::~NetLogger()
{
}

int NetLogger::WriteLog(int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, ...)
{
    char szLogContent[gc_uiMaxLogLineLen] = {0};
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(szLogContent, gc_uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    m_pLog->WriteLog(iLev, szFileName, uiFileLine, szFunction, szLogContent);

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    if (m_bEnableNetLogger && m_pLabor)
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        oTraceLog.set_node_id(m_pLabor->GetNodeId());
        oTraceLog.set_node_identify(m_pLabor->GetNodeIdentify());
        oTraceLog.set_code_file_name(szFileName);
        oTraceLog.set_code_file_line(uiFileLine);
        oTraceLog.set_code_function(szFunction);
        oTraceLog.set_log_content(szLogContent);
        oMsgBody.set_data(oTraceLog.SerializeAsString());
        oMsgBody.mutable_req_target()->set_route(m_pLabor->GetNodeIdentify());
        m_pLabor->SendOriented("LOGGER", CMD_REQ_LOG4_TRACE, 0, oMsgBody);
    }

    return 0;
}

int NetLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, ...)
{
    char szLogContent[gc_uiMaxLogLineLen] = {0};
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(szLogContent, gc_uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    if (strTraceId == m_pLabor->GetNodeIdentify())
    {
        m_pLog->WriteLog(iLev, szFileName, uiFileLine, szFunction, szLogContent);
    }
    else
    {
        m_pLog->WriteLog(strTraceId, iLev, szFileName, uiFileLine, szFunction, szLogContent);
    }
    if (m_bEnableNetLogger && m_pLabor)
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        oTraceLog.set_node_id(m_pLabor->GetNodeId());
        oTraceLog.set_node_identify(m_pLabor->GetNodeIdentify());
        oTraceLog.set_code_file_name(szFileName);
        oTraceLog.set_code_file_line(uiFileLine);
        oTraceLog.set_code_function(szFunction);
        oTraceLog.set_log_content(szLogContent);
        oMsgBody.set_trace_id(strTraceId);
        oMsgBody.set_data(oTraceLog.SerializeAsString());
        oMsgBody.mutable_req_target()->set_route(strTraceId);
        m_pLabor->SendOriented("LOGGER", CMD_REQ_LOG4_TRACE, 0, oMsgBody);
    }

    return 0;
}

} /* namespace neb */
