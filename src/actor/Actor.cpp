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
      m_uiSequence(0), m_dActiveTime(0.0), m_dTimeout(dTimeout),
      m_pWorker(nullptr), m_pTimerWatcher(NULL), m_pContext(nullptr)
{
}

Actor::~Actor()
{
    FREE(m_pTimerWatcher);
    m_pWorker->Logger(m_strTraceId, Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "eActorType %d, seq %u", m_eActorType, m_uiSequence);
}

uint32 Actor::GetSequence()
{
    if ((ACT_CMD == m_eActorType || ACT_MODULE == m_eActorType) // Cmd和Module总是获取最新Seq
        || 0 == m_uiSequence)
    {
        if (nullptr != m_pWorker)
        {
            m_uiSequence = m_pWorker->GetSequence();
        }
    }
    return(m_uiSequence);
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
std::shared_ptr<Session> Actor::GetSession(uint32 uiSessionId)
{
    return(m_pWorker->GetSession(uiSessionId));
}

std::shared_ptr<Session> Actor::GetSession(const std::string& strSessionId)
{
    return(m_pWorker->GetSession(strSessionId));
}

bool Actor::ExecStep(uint32 uiStepSeq, int iErrno, const std::string& strErrMsg, void* data)
{
    return(m_pWorker->ExecStep(uiStepSeq, iErrno, strErrMsg, data));
}

std::shared_ptr<Matrix> Actor::GetMatrix(const std::string& strMatrixName)
{
    return(m_pWorker->GetMatrix(strMatrixName));
}

std::shared_ptr<Context> Actor::GetContext()
{
    return(m_pContext);
}

void Actor::SetContext(std::shared_ptr<Context> pContext)
{
    m_pContext = pContext;
}

void Actor::AddAssemblyLine(std::shared_ptr<Session> pSession)
{
    m_pWorker->AddAssemblyLine(pSession);
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    return(m_pWorker->SendTo(pChannel));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(pChannel, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(pChannel, oHttpMsg));
}

bool Actor::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(strIdentify, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(strHost, iPort, strUrlPath, oHttpMsg, this->GetSequence()));
}

bool Actor::SendTo(std::shared_ptr<RedisChannel> pChannel)
{
    return(m_pWorker->SendTo(pChannel, this));
}

bool Actor::SendTo(const std::string& strIdentify)
{
    return(m_pWorker->SendTo(strIdentify, this));
}

bool Actor::SendTo(const std::string& strHost, int iPort)
{
    return(m_pWorker->SendTo(strHost, iPort, this));
}

bool Actor::SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendOriented(strNodeType, uiFactor, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendOriented(strNodeType, iCmd, uiSeq, oMsgBody, this));
}

void Actor::SetWorker(Worker* pWorker)
{
    m_pWorker = pWorker;
}

ev_timer* Actor::MutableTimerWatcher()
{
    if (NULL == m_pTimerWatcher)
    {
        m_pTimerWatcher = (ev_timer*)malloc(sizeof(ev_timer));
        if (NULL != m_pTimerWatcher)
        {
            memset(m_pTimerWatcher, 0, sizeof(ev_timer));
            m_pTimerWatcher->data = this;    // (void*)(Actor*)
        }
    }
    return(m_pTimerWatcher);
}

void Actor::SetActorName(const std::string& strActorName)
{
    m_strActorName = strActorName;
}

void Actor::SetTraceId(const std::string& strTraceId)
{
    m_strTraceId = strTraceId;
}

} /* namespace neb */
