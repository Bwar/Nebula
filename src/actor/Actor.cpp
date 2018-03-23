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
      m_ulSequence(0), m_ulSessionId(0), m_ulTraceId(0), m_dActiveTime(0.0), m_dTimeout(dTimeout),
      m_pWorker(NULL), m_pLogger(NULL), m_pTimerWatcher(NULL)
{
}

Actor::~Actor()
{
}

bool Actor::Register(Actor* pActor, ev_tstamp dTimeout)
{
    switch (pActor->GetActorType())
    {
        case ACT_PB_STEP:
        case ACT_HTTP_STEP:
        case ACT_REDIS_STEP:
            if (ACT_PB_STEP == GetActorType() || ACT_HTTP_STEP == GetActorType() || ACT_REDIS_STEP ==GetActorType())
            {
                pActor->SetTraceId(GetTraceId());
                return(m_pWorker->Register(GetSequence(), (Step*)pActor, dTimeout));
            }
            else
            {
                pActor->SetTraceId(m_pWorker->GetSequence());
                return(m_pWorker->Register((Step*)pActor, dTimeout));
            }
            break;
        case ACT_SESSION:
            return(m_pWorker->Register((Session*)pActor));
            break;
        case ACT_TIMER:
            break;
        default:
            return(m_pWorker->Pretreat(pActor));
    }
    return(false);
}

void Actor::Remove(Actor* pActor)
{
    switch (pActor->GetActorType())
    {
        case ACT_PB_STEP:
            if (ACT_PB_STEP == GetActorType() || ACT_HTTP_STEP == GetActorType() || ACT_REDIS_STEP ==GetActorType())
            {
                return(m_pWorker->Remove(GetSequence(), (Step*)pActor));
            }
            else
            {
                return(m_pWorker->Remove((Step*)pActor));
            }
            break;
        case ACT_HTTP_STEP:
            break;
        case ACT_REDIS_STEP:
            break;
        case ACT_SESSION:
            return(m_pWorker->Remove((Session*)pActor));
            break;
        case ACT_TIMER:
            break;
        default:
            LOG4_WARN("the Actor can not be removed.");
            break;
    }
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

const std::string& Actor::GetNodeType() const
{
    return(m_pWorker->GetNodeType());
}

const std::string& Actor::GetWorkPath() const
{
    return(m_pWorker->GetWorkPath());
}

const std::string& Actor::GetWorkerIdentify()
{
    return(m_pWorker->GetWorkerIdentify());
}

const CJsonObject& Actor::GetCustomConf() const
{
    return(m_pWorker->GetCustomConf());
}

time_t Actor::GetNowTime() const
{
    return(m_pWorker->GetNowTime());
}

Session* Actor::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pWorker->GetSession(uiSessionId, strSessionClass));
}

Session* Actor::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pWorker->GetSession(strSessionId, strSessionClass));
}

bool Actor::AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx)
{
    return(m_pWorker->AddNamedChannel(strIdentify, stCtx));
}

void Actor::DelNamedChannel(const std::string& strIdentify)
{
    m_pWorker->DelNamedChannel(strIdentify);
}

void Actor::AddInnerChannel(const tagChannelContext& stCtx)
{
    m_pWorker->AddInnerChannel(stCtx);
}

void Actor::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pWorker->AddNodeIdentify(strNodeType, strIdentify);
}

void Actor::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pWorker->DelNodeIdentify(strNodeType, strIdentify);
}

bool Actor::SendTo(const tagChannelContext& stCtx)
{
    return(m_pWorker->SendTo(stCtx));
}

bool Actor::SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(stCtx, uiCmd, uiSeq, oMsgBody));
}

bool Actor::SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(stCtx, oHttpMsg));
}

bool Actor::SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendTo(strIdentify, uiCmd, uiSeq, oMsgBody));
}

bool Actor::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg)
{
    return(m_pWorker->SendTo(strHost, iPort, strUrlPath, oHttpMsg, this->GetSequence()));
}

bool Actor::SendToNext(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendToNext(strNodeType, uiCmd, uiSeq, oMsgBody));
}

bool Actor::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pWorker->SendToWithMod(strNodeType, uiModFactor, uiCmd, uiSeq, oMsgBody));
}

void Actor::SetNodeId(uint32 uiNodeId)
{
    m_pWorker->SetNodeId(uiNodeId);
}

void Actor::DelayTimeout()
{
    if (IsRegistered())
    {
        m_pWorker->ResetTimeout(this);
    }
    else
    {
        m_dActiveTime += m_dTimeout + 0.5;
    }
}

bool Actor::SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType)
{
    return(m_pWorker->SwitchCodec(stCtx, eCodecType));
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
