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
        unsigned int uiMaxRollFileIndex, unsigned int uiMaxLogLineLen, bool bAlwaysFlush, Labor* pLabor)
    : m_iLogLevel(iLogLev), m_iNetLogLevel(Logger::INFO), m_uiMaxLogLineLen(uiMaxFileSize),
      m_bEnableNetLogger(false), m_pLabor(pLabor), m_pLog(nullptr)
{
#if __cplusplus >= 201401L
    m_pLog = std::make_unique<neb::FileLogger>(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex, bAlwaysFlush);
#else
    m_pLog = std::unique_ptr<FileLogger>(new FileLogger(strLogFile, iLogLev, uiMaxFileSize, uiMaxRollFileIndex, bAlwaysFlush));
#endif
}

NetLogger::~NetLogger()
{
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
        oTraceLog.SerializeToString(&m_strLogData);
        oMsgBody.set_data(m_strLogData);
        m_pLabor->AddNetLogMsg(oMsgBody);
    }
}

} /* namespace neb */

