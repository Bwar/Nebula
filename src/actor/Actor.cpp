/*******************************************************************************
 * Project:  Nebula
 * @file     Actor.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月9日
 * @note
 * Modify history:
 ******************************************************************************/

#include "actor/Actor.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/session/Session.hpp"
#include "actor/step/Step.hpp"
#include "labor/Worker.hpp"

namespace neb
{

Actor::Actor(ACTOR_TYPE eActorType, ev_tstamp dTimeout)
    : m_eActorType(eActorType),
      m_uiSequence(0), m_dActiveTime(0.0), m_dTimeout(dTimeout),
      m_pLabor(nullptr), m_pTimerWatcher(NULL), m_pContext(nullptr)
{
}

Actor::~Actor()
{
    FREE(m_pTimerWatcher);
    LOG4_TRACE("eActorType %d, seq %u, actor name \"%s\"",
            m_eActorType, GetSequence(), m_strActorName.c_str());
}

uint32 Actor::GetSequence()
{
    if ((ACT_CMD == m_eActorType || ACT_MODULE == m_eActorType) // Cmd和Module总是获取最新Seq
        || 0 == m_uiSequence)
    {
        if (nullptr != m_pLabor)
        {
            m_uiSequence = m_pLabor->GetSequence();
        }
    }
    return(m_uiSequence);
}

uint32 Actor::GetNodeId() const
{
    return(m_pLabor->GetNodeInfo().uiNodeId);
}

uint32 Actor::GetWorkerIndex() const
{
    return(((Worker*)m_pLabor)->GetWorkerInfo().iWorkerIndex);
}

const std::string& Actor::GetNodeType() const
{
    return(m_pLabor->GetNodeInfo().strNodeType);
}

const std::string& Actor::GetWorkPath() const
{
    return(m_pLabor->GetNodeInfo().strWorkPath);
}

const std::string& Actor::GetNodeIdentify() const
{
    return(m_pLabor->GetNodeInfo().strNodeIdentify);
}

ev_tstamp Actor::GetDataReportInterval() const
{
    return(m_pLabor->GetNodeInfo().dDataReportInterval);
}

time_t Actor::GetNowTime() const
{
    return(m_pLabor->GetNowTime());
}

const CJsonObject& Actor::GetCustomConf() const
{
    return(((Worker*)m_pLabor)->GetCustomConf());
}
std::shared_ptr<Session> Actor::GetSession(uint32 uiSessionId)
{
    return(m_pLabor->GetActorBuilder()->GetSession(uiSessionId));
}

std::shared_ptr<Session> Actor::GetSession(const std::string& strSessionId)
{
    return(m_pLabor->GetActorBuilder()->GetSession(strSessionId));
}

bool Actor::ExecStep(uint32 uiStepSeq, int iErrno, const std::string& strErrMsg, void* data)
{
    return(m_pLabor->GetActorBuilder()->ExecStep(uiStepSeq, iErrno, strErrMsg, data));
}

std::shared_ptr<Model> Actor::GetModel(const std::string& strModelName)
{
    return(m_pLabor->GetActorBuilder()->GetModel(strModelName));
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
    m_pLabor->GetActorBuilder()->AddAssemblyLine(pSession);
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    return(m_pLabor->GetDispatcher()->SendTo(pChannel));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendTo(pChannel, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    return(m_pLabor->GetDispatcher()->SendTo(pChannel, oHttpMsg));
}

bool Actor::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendTo(strIdentify, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg)
{
    return(m_pLabor->GetDispatcher()->SendTo(strHost, iPort, strUrlPath, oHttpMsg, this->GetSequence()));
}

bool Actor::SendTo(std::shared_ptr<RedisChannel> pChannel)
{
    return(m_pLabor->GetDispatcher()->SendTo(pChannel, this));
}

bool Actor::SendTo(const std::string& strIdentify)
{
    return(m_pLabor->GetDispatcher()->SendTo(strIdentify, this));
}

bool Actor::SendTo(const std::string& strHost, int iPort)
{
    return(m_pLabor->GetDispatcher()->SendTo(strHost, iPort, this));
}

bool Actor::SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendTo(iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendOriented(strNodeType, uiFactor, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendOriented(strNodeType, iCmd, uiSeq, oMsgBody, this));
}

bool Actor::SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(m_pLabor->GetDispatcher()->SendDataReport(iCmd, uiSeq, oMsgBody, this));
}

int32 Actor::GetStepNum() const
{
    return(m_pLabor->GetActorBuilder()->GetStepNum());
}

void Actor::SetLabor(Labor* pLabor)
{
    m_pLabor = pLabor;
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
