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
#include "labor/NodeInfo.hpp"
#include "labor/Labor.hpp"
#include "actor/cmd/CW.hpp"
#include "NetLogger.hpp"

namespace neb
{

NetLogger::NetLogger(const std::string strLogFile, int iLogLev, unsigned int uiMaxFileSize,
        unsigned int uiMaxRollFileIndex, unsigned int uiMaxLogLineLen, Labor* pLabor)
    : m_pLogBuff(NULL), m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_uiMaxLogLineLen(uiMaxFileSize),
      m_bEnableNetLogger(false), m_pLabor(pLabor), m_pLog(nullptr)
{
    m_pLogBuff = (char*)malloc(m_uiMaxLogLineLen);
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
    if (iLev > m_iLogLevel && iLev > m_iNetLogLevel)
    {
        return(0);
    }
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(m_pLogBuff, m_uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    m_pLog->WriteLog(iLev, szFileName, uiFileLine, szFunction, m_pLogBuff);

    if (iLev > m_iNetLogLevel)
    {
        return 0;
    }
    if (m_bEnableNetLogger && m_pLabor)  // TODO 当前版本Manager进程的日志不会发送到LOGGER，因为Manager并未存储除Beacon之外的节点信息
    {
        MsgBody oMsgBody;
        TraceLog oTraceLog;
        //oTraceLog.set_log_time();
        oTraceLog.set_node_type(m_pLabor->GetNodeInfo().strNodeType);
        oTraceLog.set_node_identify(m_pLabor->GetNodeInfo().strNodeIdentify);
        oTraceLog.set_log_level(LogLevMsg[iLev]);
        oTraceLog.set_code_file_name(szFileName);
        oTraceLog.set_code_file_line(uiFileLine);
        oTraceLog.set_code_function(szFunction);
        oTraceLog.set_log_content(m_pLogBuff);
        //oMsgBody.set_data(oTraceLog.SerializeAsString());
        oTraceLog.SerializeToString(&m_strLogData);
        oMsgBody.set_data(m_strLogData);
        oMsgBody.mutable_req_target()->set_route(m_pLabor->GetNodeInfo().strNodeIdentify);
        m_pLabor->AddNetLogMsg(oMsgBody);
    }

    return 0;
}

int NetLogger::WriteLog(const std::string& strTraceId, int iLev, const char* szFileName, unsigned int uiFileLine, const char* szFunction, const char* szLogStr, ...)
{
    va_list ap;
    va_start(ap, szLogStr);
    vsnprintf(m_pLogBuff, gc_uiMaxLogLineLen, szLogStr, ap);
    va_end(ap);

    if (strTraceId == m_pLabor->GetNodeInfo().strNodeIdentify)
    {
        m_pLog->WriteLog(iLev, szFileName, uiFileLine, szFunction, m_pLogBuff);
    }
    else
    {
        m_pLog->WriteLog(strTraceId, iLev, szFileName, uiFileLine, szFunction, m_pLogBuff);
    }
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
        oTraceLog.set_log_content(m_pLogBuff);
        oMsgBody.set_trace_id(strTraceId);
        //oMsgBody.set_data(oTraceLog.SerializeAsString());
        oTraceLog.SerializeToString(&m_strLogData);
        oMsgBody.set_data(m_strLogData);
        if (strTraceId.size() > 0)
        {
            oMsgBody.mutable_req_target()->set_route(strTraceId);
        }
        else
        {
            oMsgBody.mutable_req_target()->set_route(m_pLabor->GetNodeInfo().strNodeIdentify);
        }
        m_pLabor->AddNetLogMsg(oMsgBody);
    }

    return 0;
}

} /* namespace neb */
