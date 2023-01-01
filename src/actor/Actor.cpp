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
#include <algorithm>
#include "ios/ActorWatcher.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/IO.hpp"
#include "actor/session/Session.hpp"
#include "actor/step/Step.hpp"
#include "labor/Worker.hpp"

namespace neb
{

Actor::Actor(ACTOR_TYPE eActorType, ev_tstamp dTimeout)
    : m_eActorType(eActorType), m_uiActorStatus(ACT_STATUS_UNREGISTER),
      m_uiSequence(0), m_uiPeerStepSeq(0), m_dActiveTime(0.0), m_dTimeout(dTimeout),
      m_pLabor(nullptr), m_pWatcher(nullptr), m_pContext(nullptr)
{
}

Actor::~Actor()
{
    if (m_pWatcher != nullptr)
    {
        DELETE(m_pWatcher);
    }
    LOG4_TRACE("eActorType %d, seq %u, actor name \"%s\"",
            m_eActorType, GetSequence(), m_strActorName.c_str());
}

bool Actor::RegisterActor(const std::string& strActorName, Actor* pNewActor, Actor* pCreator)
{
    return(m_pLabor->GetActorBuilder()->RegisterActor(strActorName, pNewActor, pCreator));
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

uint32 Actor::GetLaborId() const
{
    if (Labor::LABOR_MANAGER == m_pLabor->GetLaborType())
    {
        return(m_pLabor->GetNodeInfo().uiWorkerNum + 1);
    }
    else
    {
        return(((Worker*)m_pLabor)->GetWorkerInfo().iWorkerIndex);
    }
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

const NodeInfo& Actor::GetNodeInfo() const
{
    return(m_pLabor->GetNodeInfo());
}

ev_tstamp Actor::GetDataReportInterval() const
{
    return(m_pLabor->GetNodeInfo().dDataReportInterval);
}

time_t Actor::GetNowTime() const
{
    return(m_pLabor->GetNowTime());
}

long Actor::GetNowTimeMs() const
{
    return(m_pLabor->GetNowTimeMs());
}

const CJsonObject& Actor::GetCustomConf() const
{
    return(m_pLabor->GetCustomConf());
}

std::shared_ptr<Session> Actor::GetSession(uint32 uiSessionId)
{
    auto pSession = m_pLabor->GetActorBuilder()->GetSession(uiSessionId);
    if (pSession == nullptr)
    {
        auto pActorBuilder = m_pLabor->GetLoaderActorBuilder();
        if (pActorBuilder != nullptr)
        {
            return(pActorBuilder->GetSession(uiSessionId));
        }
    }
    return(pSession);
}

std::shared_ptr<Session> Actor::GetSession(const std::string& strSessionId)
{
    auto pSession = m_pLabor->GetActorBuilder()->GetSession(strSessionId);
    if (pSession == nullptr)
    {
        auto pActorBuilder = m_pLabor->GetLoaderActorBuilder();
        if (pActorBuilder != nullptr)
        {
            return(pActorBuilder->GetSession(strSessionId));
        }
    }
    return(pSession);
}

bool Actor::ExecStep(uint32 uiStepSeq, int iErrno, const std::string& strErrMsg, void* data)
{
    return(m_pLabor->GetActorBuilder()->ExecStep(uiStepSeq, iErrno, strErrMsg, data));
}

std::shared_ptr<Operator> Actor::GetOperator(const std::string& strOperatorName)
{
    return(m_pLabor->GetActorBuilder()->GetOperator(strOperatorName));
}

std::shared_ptr<Context> Actor::GetContext()
{
    return(m_pContext);
}

void Actor::SetContext(std::shared_ptr<Context> pContext)
{
    m_pContext = pContext;
}

uint32 Actor::GetPeerStepSeq() const
{
    return(m_uiPeerStepSeq);
}

void Actor::SetPeerStepSeq(uint32 uiPeerStepSeq)
{
    m_uiPeerStepSeq = uiPeerStepSeq;
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    return(IO<CodecNebula>::Send(pChannel));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(IO<CodecNebula>::SendResponse(this, pChannel, iCmd, uiSeq, oMsgBody));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"x-trace-id", GetTraceId()});
    return(IO<CodecHttp>::SendResponse(this, pChannel, oHttpMsg));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const RedisReply& oRedisReply)
{
    return(IO<CodecResp>::SendResponse(this, pChannel, oRedisReply));
}

bool Actor::SendTo(std::shared_ptr<SocketChannel> pChannel, const char* pRawData, uint32 uiRawDataSize)
{
    return(IO<CodecRaw>::SendResponse(this, pChannel, pRawData, uiRawDataSize));
}

bool Actor::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(IO<CodecNebula>::SendTo(this, strIdentify, SOCKET_STREAM, false, true, iCmd, uiSeq, oMsgBody)); 
}

bool Actor::SendTo(const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq)
{
    bool bWithSsl = false;
    bool bPipeline = false;
    if (oHttpMsg.headers().find("x-trace-id") == oHttpMsg.headers().end())
    {
        (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"x-trace-id", GetTraceId()});
    }
    if (oHttpMsg.http_major() >= 2)
    {
        bPipeline = true;
    }
    std::string strSchema = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(":"));
    std::transform(strSchema.begin(), strSchema.end(), strSchema.begin(), [](unsigned char c)->unsigned char {return std::tolower(c);});
    if (strSchema == std::string("https"))
    {
        bWithSsl = true;
    }
    if (oHttpMsg.http_major() == 2)
    {
        return(IO<CodecHttp2>::SendTo(this, strHost, iPort, SOCKET_STREAM, bWithSsl, bPipeline, oHttpMsg));
    }
    else
    {
        return(IO<CodecHttp>::SendTo(this, strHost, iPort, SOCKET_STREAM, bWithSsl, bPipeline, oHttpMsg));
    }
}

bool Actor::SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    return(IO<CodecResp>::SendTo(this, strIdentify, SOCKET_STREAM, bWithSsl, bPipeline, oRedisMsg));
}

bool Actor::SendToCluster(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline, bool bEnableReadOnly)
{
    return(m_pLabor->GetActorBuilder()->SendToCluster(strIdentify, bWithSsl, bPipeline, oRedisMsg, GetSequence(), bEnableReadOnly));
}

bool Actor::SendRoundRobin(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline)
{
    return(IO<CodecResp>::SendRoundRobin(this, strIdentify, bWithSsl, bPipeline, oRedisMsg));
}

bool Actor::SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(m_pLabor->GetDispatcher()->SendTo(iCmd, uiSeq, oMsgBody));
}

bool Actor::SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(IO<CodecNebula>::SendRoundRobin(this, strNodeType, false, true, iCmd, uiSeq, oMsgBody));
}

bool Actor::SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(IO<CodecNebula>::SendOriented(this, strNodeType, false, true, uiFactor, iCmd, uiSeq, oMsgBody));
}

bool Actor::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    if (oMsgBody.has_req_target())
    {
        if (0 != oMsgBody.req_target().route_id())
        {
            return(SendOriented(strNodeType, oMsgBody.req_target().route_id(), iCmd, uiSeq, oMsgBody));
        }
        else if (oMsgBody.req_target().route().length() > 0)
        {
            return(IO<CodecNebula>::SendOriented(this, strNodeType, false, true, oMsgBody.req_target().route(), iCmd, uiSeq, oMsgBody));
        }
        else
        {
            return(SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody));
        }
    }
    else
    {
        LOG4_ERROR("the request message has no req_target.");
        return(SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody));
    }
}

bool Actor::SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(GetTraceId());
    return(m_pLabor->GetDispatcher()->SendDataReport(iCmd, uiSeq, oMsgBody));
}

void Actor::CircuitBreak(const std::string& strIdentify)
{
    m_pLabor->GetDispatcher()->CircuitBreak(strIdentify);
}

uint32 Actor::SendToSelf(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    return(IO<CodecNebula>::SendToSelf(this, iCmd, uiSeq, oMsgBody));
}

uint32 Actor::SendToSelf(const HttpMsg& oHttpMsg)
{
    return(IO<CodecHttp>::SendToSelf(this, oHttpMsg));
}

uint32 Actor::SendToSelf(const RedisMsg& oRedisMsg)
{
    return(IO<CodecResp>::SendToSelf(this, oRedisMsg));
}

std::shared_ptr<SocketChannel> Actor::GetLastActivityChannel()
{
    return(m_pLabor->GetDispatcher()->GetLastActivityChannel());
}

bool Actor::CloseRawChannel(std::shared_ptr<SocketChannel> pChannel)
{
    if (CODEC_UNKNOW == pChannel->GetCodecType())
    {
        m_pLabor->GetDispatcher()->Disconnect(pChannel, false);
    }
    return(false);
}

int32 Actor::GetStepNum() const
{
    return(m_pLabor->GetActorBuilder()->GetStepNum());
}

void Actor::ResetTimeout(ev_tstamp dTimeout)
{
    m_dTimeout = dTimeout;
}

void Actor::SetLabor(Labor* pLabor)
{
    m_pLabor = pLabor;
}

uint32 Actor::ForceNewSequence()
{
    if (nullptr != m_pLabor)
    {
        m_uiSequence = m_pLabor->GetSequence();
    }
    return(m_uiSequence);
}

ActorWatcher* Actor::MutableWatcher()
{
    if (nullptr == m_pWatcher)
    {
        try
        {
            m_pWatcher = new ActorWatcher();
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new ActorWatcher error: %s", e.what());
        }
    }
    return(m_pWatcher);
}

void Actor::SetActorName(const std::string& strActorName)
{
    m_strActorName = strActorName;
}

void Actor::SetTraceId(const std::string& strTraceId)
{
    m_strTraceId = strTraceId;
}

void Actor::SetActorStatus(uint32 uiActorStatus)
{
    m_uiActorStatus |= uiActorStatus;
}

void Actor::UnsetActorStatus(uint32 uiActorStatus)
{
    m_uiActorStatus &= (~uiActorStatus);
}

} /* namespace neb */
