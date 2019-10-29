/*******************************************************************************
 * Project:  Nebula
 * @file     SessionLogger.cpp
 * @brief 
 * @author   bwar
 * @date:    Sep 20, 2016
 * @note
 * Modify history:
 ******************************************************************************/

#include "SessionLogger.hpp"
#include "actor/cmd/CW.hpp"

namespace neb
{

SessionLogger::SessionLogger()
    : Timer("neb::SessionLogger", 1.0)
{
}

SessionLogger::~SessionLogger()
{
   m_listLogMsgBody.clear();
}

E_CMD_STATUS SessionLogger::Timeout()
{
    uint32 uiNeedSendNum = m_listLogMsgBody.size();
    for (uint32 i = 0; i < uiNeedSendNum; ++i)
    {
        SendOriented("LOGGER", CMD_REQ_LOG4_TRACE, GetSequence(), m_listLogMsgBody.front());
        m_listLogMsgBody.pop_front();
    }
    return(CMD_STATUS_RUNNING);
}

void SessionLogger::AddMsg(const MsgBody& oMsgBody)
{
    // 此函数不能写日志，不然可能会导致写日志函数与此函数无限递归
    m_listLogMsgBody.push_back(oMsgBody);
}

}

