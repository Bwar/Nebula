/*******************************************************************************
 * Project:  Nebula
 * @file     SessionManagerLogger.cpp
 * @brief 
 * @author   bwar
 * @date:    Sep 20, 2016
 * @note
 * Modify history:
 ******************************************************************************/

#include "labor/Manager.hpp"
#include "SessionManagerLogger.hpp"

namespace neb
{

SessionManagerLogger::SessionManagerLogger(Manager* pManager)
    : m_pManager(pManager)
{
}

SessionManagerLogger::~SessionManagerLogger()
{
   m_listLogMsgBody.clear();
}

E_CMD_STATUS SessionManagerLogger::Timeout()
{
    uint32 uiNeedSendNum = m_listLogMsgBody.size();
    for (uint32 i = 0; i < uiNeedSendNum; ++i)
    {
        m_pManager->SendOriented("LOGGER", CMD_REQ_LOG4_TRACE, 1, m_listLogMsgBody.front());
        m_listLogMsgBody.pop_front();
    }
    return(CMD_STATUS_RUNNING);
}

void SessionManagerLogger::AddMsg(const MsgBody& oMsgBody)
{
    // 此函数不能写日志，不然可能会导致写日志函数与此函数无限递归
    m_listLogMsgBody.push_back(oMsgBody);
}

}
