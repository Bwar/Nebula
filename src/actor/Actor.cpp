/*******************************************************************************
 * Project:  Nebula
 * @file     Actor.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月9日
 * @note
 * Modify history:
 ******************************************************************************/

#include "labor/Worker.hpp"
#include "actor/Actor.hpp"
#include "actor/session/Session.hpp"
#include "actor/step/Step.hpp"

namespace neb
{

Actor::Actor(ACTOR_TYPE eActorType, ev_tstamp dTimeout)
    : m_eActorType(eActorType),
      m_ulSequence(0), m_dActiveTime(0.0), m_dTimeout(dTimeout),
      m_pWorker(NULL), m_pTimerWatcher(NULL)
{
}

Actor::~Actor()
{
}

uint32 Actor::GetSequence()
{
    if (0 == m_ulSequence)
    {
        if (nullptr != m_pWorker)
        {
            m_ulSequence = m_pWorker->GetSequence();
        }
    }
    return(m_ulSequence);
}

uint32 Actor::GetNodeId() const
{
    return(m_pWorker->GetNodeId());
}

uint32 Actor::GetWorkerIndex() const
{
    return(m_pWorker->GetWorkerIndex());
}

ev_tstamp Actor::GetDefaultTimeout() const
{
    return(m_pWorker->GetDefaultTimeout());
}

const std::string& Actor::GetNodeType() const
{
    return(m_pWorker->GetNodeType());
}

const std::string& Actor::GetWorkPath() const
{
    return(m_pWorker->GetWorkPath());
}

const std::string& Actor::GetNodeIdentify() const
{
    return(m_pWorker->GetNodeIdentify());
}

time_t Actor::GetNowTime() const
{
    return(m_pWorker->GetNowTime());
}

const CJsonObject& Actor::GetCustomConf() const
{
    return(m_pWorker->GetCustomConf());
}
std::shared_ptr<Session> Actor::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pWorker->GetSession(uiSessionId, strSessionClass));
}

std::shared_ptr<Session> Actor::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pWorker->GetSession(strSessionId, strSessionClass));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    return(m_pWorker->SendTo(pChannel));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(pChannel, uiCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(pChannel, oHttpMsg));
}

bool Actor::SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(strIdentify, uiCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(strHost, iPort, strUrlPath, oHttpMsg, this->GetSequence()));
}

bool Actor::SendPolling(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendPolling(strNodeType, uiCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, uint32 uiFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendOriented(strNodeType, uiFactor, uiCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendOriented(strNodeType, uiCmd, uiSeq, oMsgBody, this));
}

void Actor::SetWorker(Worker* pWorker)
{
    m_pWorker = pWorker;
}

ev_timer* Actor::AddTimerWatcher()
{
    m_pTimerWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (NULL != m_pTimerWatcher)
    {
        m_pTimerWatcher->data = this;    // (void*)(Channel*)
    }
    return(m_pTimerWatcher);
}

} /* namespace neb */
