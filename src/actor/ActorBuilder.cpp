/*******************************************************************************
 * Project:  Nebula
 * @file     ActorBuilder.cpp
 * @brief    Actor创建和管理
 * @author   Bwar
 * @date:    2019年9月15日
 * @note
 * Modify history:
 ******************************************************************************/

#include <dlfcn.h>
#include "ActorBuilder.hpp"
#include "util/json/CJsonObject.hpp"
#include "labor/NodeInfo.hpp"
#include "Actor.hpp"
#include "cmd/Cmd.hpp"
#include "cmd/Module.hpp"
#include "session/Session.hpp"
#include "step/HttpStep.hpp"
#include "step/PbStep.hpp"
#include "step/RedisStep.hpp"
#include "step/RawStep.hpp"
//#include "step/CassStep.hpp"
#include "cmd/RedisCmd.hpp"
#include "cmd/RawCmd.hpp"
#include "operator/Operator.hpp"
#include "chain/Chain.hpp"
#include "actor/session/sys_session/SessionLogger.hpp"
#include "ios/ActorWatcher.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/IO.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/SelfChannel.hpp"
#include "codec/CodecProto.hpp"

namespace neb
{

ActorBuilder::ActorBuilder(Labor* pLabor, std::shared_ptr<NetLogger> pLogger)
    : m_pErrBuff(nullptr), m_pLabor(pLabor), m_pLogger(pLogger)
{
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
}

ActorBuilder::~ActorBuilder()
{
    m_mapCmd.clear();
    m_mapCallbackStep.clear();
    m_mapCallbackSession.clear();

    if (m_pErrBuff != nullptr)
    {
        free(m_pErrBuff);
        m_pErrBuff = nullptr;
    }
}

bool ActorBuilder::Init(CJsonObject& oBootLoadConf, CJsonObject& oDynamicLoadConf)
{
    LoadSysCmd();
    BootLoadCmd(oBootLoadConf);
    DynamicLoad(oDynamicLoadConf);
    std::string strReportSessionId = "neb::SessionDataReport";
    auto pSession = GetSession(strReportSessionId);
    if (pSession == nullptr)
    {
        MakeSharedSession(nullptr, "neb::SessionDataReport",
                strReportSessionId, (ev_tstamp)m_pLabor->GetNodeInfo().dDataReportInterval);
    }
    return(true);
}

bool ActorBuilder::Init(CJsonObject& oDynamicLoadConf)
{
    DynamicLoad(oDynamicLoadConf);
    return(true);
}

void ActorBuilder::StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<ActorWatcher*>(watcher->data);
        auto pStep = std::static_pointer_cast<Step>(pWatcher->GetActor());
        pStep->m_pLabor->GetActorBuilder()->OnStepTimeout(pStep);
    }
}

void ActorBuilder::SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<ActorWatcher*>(watcher->data);
        auto pSession = std::static_pointer_cast<Session>(pWatcher->GetActor());
        pSession->m_pLabor->GetActorBuilder()->OnSessionTimeout(pSession);
    }
}

void ActorBuilder::ChainTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        auto pWatcher = static_cast<ActorWatcher*>(watcher->data);
        auto pChain = std::static_pointer_cast<Chain>(pWatcher->GetActor());
        pChain->m_pLabor->GetActorBuilder()->OnChainTimeout(pChain);
    }
}

bool ActorBuilder::OnStepTimeout(std::shared_ptr<Step> pStep)
{
    ev_timer* watcher = pStep->MutableWatcher()->MutableTimerWatcher();
    LOG4_TRACE("seq %lu: active_time %lf, now_time %lf, lifetime %lf",
            pStep->GetSequence(), pStep->GetActiveTime(), m_pLabor->GetNowTime(), pStep->GetTimeout());
    E_CMD_STATUS eResult = pStep->Timeout();
    if (CMD_STATUS_RUNNING == eResult)
    {
        ev_tstamp after = pStep->GetTimeout();
        m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
        return(true);
    }
    else
    {
        RemoveChain(pStep->GetChainId());
        RemoveStep(pStep);
        return(true);
    }
}

bool ActorBuilder::OnSessionTimeout(std::shared_ptr<Session> pSession)
{
    ev_timer* watcher = pSession->MutableWatcher()->MutableTimerWatcher();
    //LOG4_TRACE("CHECK watchar = 0x%x", watcher);
    ev_tstamp after = pSession->GetActiveTime() - m_pLabor->GetNowTime() + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
        return(true);
    }
    else    // 会话已超时
    {
        //LOG4_TRACE("session_id: %s", pSession->GetSessionId().c_str());
        if (CMD_STATUS_RUNNING == pSession->Timeout())
        {
            m_pLabor->GetDispatcher()->RefreshEvent(watcher, pSession->GetTimeout());
            return(true);
        }
        else
        {
            RemoveSession(pSession);
            return(true);
        }
    }
}

bool ActorBuilder::OnChainTimeout(std::shared_ptr<Chain> pChain)
{
    ev_timer* watcher = pChain->MutableWatcher()->MutableTimerWatcher();
    LOG4_TRACE("CHECK watchar = 0x%x", watcher);
    ev_tstamp after = pChain->GetActiveTime() - m_pLabor->GetNowTime() + pChain->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
        return(true);
    }
    else    // 会话已超时
    {
        if (CMD_STATUS_RUNNING == pChain->Timeout())
        {
            m_pLabor->GetDispatcher()->RefreshEvent(watcher, pChain->GetTimeout());
            return(true);
        }
        else
        {
            RemoveChain(pChain->GetSequence());
            return(true);
        }
    }
}

bool ActorBuilder::OnMessage(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_DEBUG("cmd %u, seq %u", oMsgHead.cmd(), oMsgHead.seq());
    if (gc_uiCmdReq & oMsgHead.cmd())    // 新请求
    {
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        auto cmd_iter = m_mapCmd.find(gc_uiCmdBit & oMsgHead.cmd());
        if (cmd_iter != m_mapCmd.end() && cmd_iter->second != nullptr)
        {
            if (oMsgBody.trace_id().length() > 10)
            {
                cmd_iter->second->SetTraceId(oMsgBody.trace_id());
            }
            else
            {
                std::ostringstream oss;
                oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
                cmd_iter->second->SetTraceId(oss.str());
            }
            cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
        }
        else    // 没有对应的cmd，是需由接入层转发的请求
        {
            if (CMD_REQ_SET_LOG_LEVEL == oMsgHead.cmd())
            {
                LogLevel oLogLevel;
                oLogLevel.ParseFromString(oMsgBody.data());
                LOG4_INFO("log level set to %d, net_log_level set to %d",
                        oLogLevel.log_level(), oLogLevel.net_log_level());
                m_pLogger->SetLogLevel(oLogLevel.log_level());
                m_pLogger->SetNetLogLevel(oLogLevel.net_log_level());
            }
            else if (CMD_REQ_RELOAD_SO == oMsgHead.cmd())
            {
                CJsonObject oSoConfJson;
                if (oSoConfJson.Parse(oMsgBody.data()))
                {
                    DynamicLoad(oSoConfJson);
                }
                else
                {
                    LOG4_ERROR("json parse string error: \"%s\"");
                }
            }
            else
            {
                if (CODEC_NEBULA == pChannel->GetCodecType())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                {
                    cmd_iter = m_mapCmd.find(CMD_REQ_TO_CLIENT);
                    if (cmd_iter != m_mapCmd.end())
                    {
                        if (oMsgBody.trace_id().length() > 10)
                        {
                            cmd_iter->second->SetTraceId(oMsgBody.trace_id());
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
                            cmd_iter->second->SetTraceId(oss.str());
                        }
                        cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
                    }
                    else
                    {
                        snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                        LOG4_ERROR(m_pErrBuff);
                        oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                        oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                        IO<CodecNebula>::SendResponse(m_pLabor->GetDispatcher(), pChannel, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody);
                        return(false);
                    }
                }
                else
                {
                    cmd_iter = m_mapCmd.find(CMD_REQ_FROM_CLIENT);
                    if (cmd_iter != m_mapCmd.end())
                    {
                        if (oMsgBody.trace_id().length() > 10)
                        {
                            cmd_iter->second->SetTraceId(oMsgBody.trace_id());
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
                            cmd_iter->second->SetTraceId(oss.str());
                        }
                        cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
                    }
                    else
                    {
                        snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                        LOG4_ERROR(m_pErrBuff);
                        oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                        oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                        IO<CodecNebula>::SendResponse(m_pLabor->GetDispatcher(), pChannel, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody);
                        return(false);
                    }
                }
            }
        }
    }
    else    // 回调
    {
        pChannel->m_pImpl->PopStepSeq();
        auto step_iter = m_mapCallbackStep.find(oMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",
                            oMsgHead.cmd());
            if (step_iter->second != nullptr)
            {
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(m_pLabor->GetNowTime());
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, active_time %lf",
                                oMsgHead.cmd(), oMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second->GetActiveTime());
                eResult = (std::dynamic_pointer_cast<PbStep>(step_iter->second))->Callback(pChannel, oMsgHead, oMsgBody);
                if (CMD_STATUS_RUNNING != eResult)
                {
                    uint32 uiChainId = step_iter->second->GetChainId();
                    m_pLabor->GetDispatcher()->DelEvent(step_iter->second->MutableWatcher()->MutableTimerWatcher());
                    step_iter->second->MutableWatcher()->Reset();
                    m_mapCallbackStep.erase(step_iter);
                    if (CMD_STATUS_FAULT != eResult && 0 != uiChainId)
                    {
                        auto chain_iter = m_mapChain.find(uiChainId);
                        if (chain_iter != m_mapChain.end())
                        {
                            chain_iter->second->SetActiveTime(m_pLabor->GetNowTime());
                            eResult = chain_iter->second->Next();
                            if (CMD_STATUS_RUNNING != eResult)
                            {
                                RemoveChain(uiChainId);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            return(false);
        }
    }
    return(true);
}

bool ActorBuilder::OnError(std::shared_ptr<SocketChannel> pChannel, uint32 uiStepSeq, int iErrno, const std::string& strErrMsg)
{
    auto step_iter = m_mapCallbackStep.find(uiStepSeq);
    if (step_iter != m_mapCallbackStep.end())
    {
        if (step_iter->second != nullptr)
        {
            E_CMD_STATUS eResult;
            step_iter->second->SetActiveTime(m_pLabor->GetNowTime());
            eResult = step_iter->second->ErrBack(pChannel, iErrno, strErrMsg);
            if (CMD_STATUS_RUNNING != eResult)
            {
                uint32 uiChainId = step_iter->second->GetChainId();
                m_pLabor->GetDispatcher()->DelEvent(step_iter->second->MutableWatcher()->MutableTimerWatcher());
                step_iter->second->MutableWatcher()->Reset();
                m_mapCallbackStep.erase(step_iter);
                if (CMD_STATUS_FAULT != eResult && 0 != uiChainId)
                {
                    auto chain_iter = m_mapChain.find(uiChainId);
                    if (chain_iter != m_mapChain.end())
                    {
                        chain_iter->second->SetActiveTime(m_pLabor->GetNowTime());
                        eResult = chain_iter->second->Next();
                        if (CMD_STATUS_RUNNING != eResult)
                        {
                            RemoveChain(uiChainId);
                        }
                    }
                }
            }
        }
        return(true);
    }
    else
    {
        snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %u had been timeout!", uiStepSeq);
        LOG4_WARNING(m_pErrBuff);
        return(false);
    }
}

void ActorBuilder::RemoveStep(std::shared_ptr<Step> pStep)
{
    if (nullptr == pStep)
    {
        return;
    }
    std::unordered_map<uint32, std::shared_ptr<Step> >::iterator callback_iter;
    m_pLabor->GetDispatcher()->DelEvent(pStep->MutableWatcher()->MutableTimerWatcher());
    pStep->MutableWatcher()->Reset();
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("erase step(seq %u)", pStep->GetSequence());
        m_mapCallbackStep.erase(callback_iter);
    }
}

void ActorBuilder::RemoveSession(std::shared_ptr<Session> pSession)
{
    if (nullptr == pSession)
    {
        return;
    }
    m_pLabor->GetDispatcher()->DelEvent(pSession->MutableWatcher()->MutableTimerWatcher());
    pSession->MutableWatcher()->Reset();
    auto iter = m_mapCallbackSession.find(pSession->GetSessionId());
    if (iter != m_mapCallbackSession.end())
    {
        LOG4_TRACE("erase session(session_id %s)", pSession->GetSessionId().c_str());
        m_mapCallbackSession.erase(iter);
    }
}

void ActorBuilder::RemoveChain(uint32 uiChainId)
{
    auto chain_iter = m_mapChain.find(uiChainId);
    if (chain_iter != m_mapChain.end())
    {
        std::shared_ptr<Chain> pChain = chain_iter->second;
        m_pLabor->GetDispatcher()->DelEvent(pChain->MutableWatcher()->MutableTimerWatcher());
        pChain->MutableWatcher()->Reset();
        m_mapChain.erase(chain_iter);
    }
}

void ActorBuilder::ChannelNotice(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const std::string& strClientData)
{
    LOG4_TRACE(" ");
    auto cmd_iter = m_mapCmd.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapCmd.end() && cmd_iter->second != nullptr)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        oMsgBody.mutable_req_target()->set_route_id(0);
        oMsgBody.set_data(strIdentify);
        oMsgBody.set_add_on(strClientData);
        oMsgHead.set_cmd(CMD_REQ_DISCONNECT);
        oMsgHead.set_seq(m_pLabor->GetSequence());
        oMsgHead.set_len(oMsgBody.ByteSize());
        std::ostringstream oss;
        oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
        cmd_iter->second->SetTraceId(oss.str());
        cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
    }
}

void ActorBuilder::AddChainConf(const std::string& strChainKey, std::queue<std::vector<std::string> >&& queChainBlocks)
{
    m_mapChainConf.insert(std::make_pair(strChainKey, std::move(queChainBlocks)));
}

void ActorBuilder::LoadSysCmd()
{
    // 调用MakeSharedCmd等系列函数必须严格匹配参数类型，类型不符需显式转换，如 (int)CMD_REQ_CONNECT_TO_WORKER
    MakeSharedCmd(nullptr, "neb::CmdBeat", (int)CMD_REQ_BEAT);
    if (Labor::LABOR_MANAGER == m_pLabor->GetLaborType())
    {
        MakeSharedCmd(nullptr, "neb::CmdOnWorkerLoad", (int)CMD_REQ_UPDATE_WORKER_LOAD);
        MakeSharedCmd(nullptr, "neb::CmdOnTellWorker", (int)CMD_REQ_TELL_WORKER);
        MakeSharedCmd(nullptr, "neb::CmdOnOrientationFdTransfer", (int)CMD_REQ_CONNECT_TO_WORKER);
        MakeSharedCmd(nullptr, "neb::CmdOnNodeNotice", (int)CMD_REQ_NODE_NOTICE);
        MakeSharedCmd(nullptr, "neb::CmdOnSetNodeConf", (int)CMD_REQ_SET_NODE_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnGetNodeConf", (int)CMD_REQ_GET_NODE_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnSetNodeCustomConf", (int)CMD_REQ_SET_NODE_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnGetNodeCustomConf", (int)CMD_REQ_GET_NODE_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnSetCustomConf", (int)CMD_REQ_SET_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnGetCustomConf", (int)CMD_REQ_GET_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdOnStartService", (int)CMD_REQ_START_SERVICE);
        MakeSharedCmd(nullptr, "neb::CmdDataReport", (int)CMD_REQ_DATA_REPORT);
        std::string strModulePath = "/healthy";
        MakeSharedModule(nullptr, "neb::ModuleHealth", strModulePath);
        strModulePath = "/health";
        MakeSharedModule(nullptr, "neb::ModuleHealth", strModulePath);
        strModulePath = "/status";
        MakeSharedModule(nullptr, "neb::ModuleMetrics", strModulePath);
        strModulePath = "http_upgrade";
        MakeSharedModule(nullptr, "neb::ModuleHttpUpgrade", strModulePath);
    }
    else
    {
        MakeSharedCmd(nullptr, "neb::CmdToldWorker", (int)CMD_REQ_TELL_WORKER);
        MakeSharedCmd(nullptr, "neb::CmdUpdateNodeId", (int)CMD_REQ_REFRESH_NODE_ID);
        MakeSharedCmd(nullptr, "neb::CmdNodeNotice", (int)CMD_REQ_NODE_NOTICE);
        MakeSharedCmd(nullptr, "neb::CmdSetNodeConf", (int)CMD_REQ_SET_NODE_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdSetNodeCustomConf", (int)CMD_REQ_SET_NODE_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdReloadCustomConf", (int)CMD_REQ_RELOAD_CUSTOM_CONFIG);
        std::string strModulePath = "/healthy";
        MakeSharedModule(nullptr, "neb::ModuleHealth", strModulePath);
        strModulePath = "/health";
        MakeSharedModule(nullptr, "neb::ModuleHealth", strModulePath);
        strModulePath = "/status";
        MakeSharedModule(nullptr, "neb::ModuleHealth", strModulePath);
        strModulePath = "http_upgrade";
        MakeSharedModule(nullptr, "neb::ModuleHttpUpgrade", strModulePath);
    }
    m_pSessionLogger = std::dynamic_pointer_cast<SessionLogger>(MakeSharedSession(nullptr, "neb::SessionLogger"));
}

std::shared_ptr<Actor> ActorBuilder::InitializeSharedActor(Actor* pCreator, std::shared_ptr<Actor> pSharedActor, const std::string& strActorName)
{
    pSharedActor->SetLabor(m_pLabor);
    pSharedActor->SetActiveTime(m_pLabor->GetNowTime());
    pSharedActor->SetActorName(strActorName);
    pSharedActor->SetActorStatus(Actor::ACT_STATUS_SHARED);
    if (nullptr != pCreator)
    {
        if (pSharedActor->GetActorType() != Actor::ACT_CONTEXT)
        {
            pSharedActor->SetContext(pCreator->GetContext());
        }
        pSharedActor->SetActorStatus(Actor::ACT_STATUS_DYNAMIC_LOAD | pCreator->GetActorStatus());
    }
    switch (pSharedActor->GetActorType())
    {
        case Actor::ACT_PB_STEP:
        case Actor::ACT_HTTP_STEP:
        case Actor::ACT_REDIS_STEP:
        case Actor::ACT_RAW_STEP:
            if (TransformToSharedStep(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_SESSION:
        case Actor::ACT_TIMER:
            if (TransformToSharedSession(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_CONTEXT:
            return(pSharedActor);
            break;
        case Actor::ACT_CMD:
            if (TransformToSharedCmd(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_MODULE:
            if (TransformToSharedModule(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_OPERATOR:
            if (TransformToSharedOperator(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_CHAIN:
            if (TransformToSharedChain(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        default:
            LOG4_ERROR("\"%s\" must be a Step, a Session, an Operator, a Cmd or a Module.",
                    strActorName.c_str());
            return(nullptr);
    }
    return(nullptr);
}

bool ActorBuilder::TransformToSharedStep(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    pSharedActor->m_dTimeout = (gc_dConfigTimeout == pSharedActor->m_dTimeout)
            ? m_pLabor->GetNodeInfo().dStepTimeout : pSharedActor->m_dTimeout;
    auto pWatcher = pSharedActor->MutableWatcher();
    if (nullptr == pWatcher)
    {
        return(false);
    }
    pWatcher->Set(pSharedActor);
    ev_timer* timer_watcher = pWatcher->MutableTimerWatcher();
    if (nullptr == timer_watcher)
    {
        return(false);
    }

    if (nullptr != pCreator)
    {
        pSharedActor->SetTraceId(pCreator->GetTraceId());
    }

    while (m_mapCallbackStep.find(pSharedActor->GetSequence()) != m_mapCallbackStep.end())
    {
        pSharedActor->ForceNewSequence();
    }
    std::shared_ptr<Step> pSharedStep = std::dynamic_pointer_cast<Step>(pSharedActor);

    auto ret = m_mapCallbackStep.insert(std::make_pair(pSharedStep->GetSequence(), pSharedStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pSharedStep->m_dTimeout)
        {
            m_pLabor->GetDispatcher()->AddEvent(timer_watcher, StepTimeoutCallback, pSharedStep->m_dTimeout);
        }
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSharedStep->GetSequence(), pSharedStep->GetActiveTime(), pSharedStep->GetTimeout());
        return(true);
    }
    else
    {
        LOG4_ERROR("step %u exist.", pSharedStep->GetSequence());
        return(false);
    }
}

bool ActorBuilder::TransformToSharedSession(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    auto pWatcher = pSharedActor->MutableWatcher();
    if (nullptr == pWatcher)
    {
        return(false);
    }
    pWatcher->Set(pSharedActor);
    ev_timer* timer_watcher = pWatcher->MutableTimerWatcher();
    if (nullptr == timer_watcher)
    {
        return(false);
    }

    if (nullptr != pCreator)
    {
        std::ostringstream oss;
        oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << pSharedActor->GetSequence();
        pSharedActor->SetTraceId(oss.str());
    }
    std::shared_ptr<Session> pSharedSession = std::dynamic_pointer_cast<Session>(pSharedActor);
    auto ret = m_mapCallbackSession.insert(std::make_pair(pSharedSession->GetSessionId(), pSharedSession));
    if (ret.second)
    {
        if (pSharedSession->m_dTimeout > 0)
        {
            m_pLabor->GetDispatcher()->AddEvent(timer_watcher, SessionTimeoutCallback, pSharedSession->m_dTimeout);
        }
        return(true);
    }
    else
    {
        LOG4_ERROR("session \"%s\" exist.", pSharedSession->GetSessionId().c_str());
        return(false);
    }
}

bool ActorBuilder::TransformToSharedCmd(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    if (nullptr != pCreator)
    {
        pSharedActor->SetTraceId(pCreator->GetTraceId());
    }
    std::shared_ptr<Cmd> pSharedCmd = std::dynamic_pointer_cast<Cmd>(pSharedActor);
    auto ret = m_mapCmd.insert(std::make_pair(pSharedCmd->GetCmd(), pSharedCmd));
    if (ret.second)
    {
        if (pSharedCmd->Init())
        {
            return(true);
        }
        else
        {
            LOG4_ERROR("%s(%d) Init failed.", pSharedCmd->GetActorName().c_str(), pSharedCmd->GetCmd());
            m_mapCmd.erase(ret.first);
        }
    }
    else
    {
        LOG4_ERROR("cmd %d exist.", pSharedCmd->GetCmd());
    }
    return(false);
}

bool ActorBuilder::TransformToSharedModule(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    if (nullptr != pCreator)
    {
        pSharedActor->SetTraceId(pCreator->GetTraceId());
    }

    std::shared_ptr<Module> pSharedModule = std::dynamic_pointer_cast<Module>(pSharedActor);
    auto ret = m_mapModule.insert(std::make_pair(pSharedModule->GetModulePath(), pSharedModule));
    if (ret.second)
    {
        if (pSharedModule->Init())
        {
            return(true);
        }
        else
        {
            LOG4_ERROR("%s(%s) Init failed.", pSharedModule->GetActorName().c_str(), pSharedModule->GetModulePath().c_str());
            m_mapModule.erase(ret.first);
        }
    }
    else
    {
        LOG4_ERROR("module path \"%s\" exist.", pSharedModule->GetModulePath().c_str());
    }
    return(false);
}

bool ActorBuilder::TransformToSharedOperator(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    std::shared_ptr<Operator> pSharedOperator = std::dynamic_pointer_cast<Operator>(pSharedActor);
    auto ret = m_mapOperator.insert(std::make_pair(pSharedOperator->GetActorName(), pSharedOperator));
    if (ret.second)
    {
        if (pSharedOperator->Init())
        {
            return(true);
        }
        else
        {
            LOG4_ERROR("%s Init failed.", pSharedOperator->GetActorName().c_str());
            m_mapOperator.erase(ret.first);
        }
    }
    else
    {
        LOG4_ERROR("operator \"%s\" exist.", pSharedOperator->GetActorName().c_str());
    }
    return(false);
}

bool ActorBuilder::TransformToSharedChain(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    auto pWatcher = pSharedActor->MutableWatcher();
    if (nullptr == pWatcher)
    {
        return(false);
    }
    pWatcher->Set(pSharedActor);
    ev_timer* timer_watcher = pWatcher->MutableTimerWatcher();
    if (nullptr == timer_watcher)
    {
        return(false);
    }

    if (nullptr != pCreator)
    {
        pSharedActor->SetTraceId(pCreator->GetTraceId());
    }

    std::shared_ptr<Chain> pSharedChain = std::dynamic_pointer_cast<Chain>(pSharedActor);
    auto chain_conf_iter = m_mapChainConf.find(pSharedChain->GetChainFlag());
    if (chain_conf_iter == m_mapChainConf.end())
    {
        neb::CJsonObject oChainConf;
        if (oChainConf.Parse(pSharedChain->GetChainFlag()))
        {
            if (!pSharedChain->Init(oChainConf))
            {
                LOG4_ERROR("chain \"%s\" init failed!", pSharedChain->GetActorName().c_str());
                return(false);
            }
        }
        else
        {
            LOG4_ERROR("no chain block config for \"%s\"", pSharedChain->GetChainFlag().c_str());
            return(false);
        }
    }
    else
    {
        if (!pSharedChain->Init(chain_conf_iter->second))
        {
            LOG4_ERROR("chain \"%s\" init failed!", pSharedChain->GetActorName().c_str());
            return(false);
        }
    }
    auto ret = m_mapChain.insert(std::make_pair(pSharedChain->GetSequence(), pSharedChain));
    if (ret.second)
    {
        if (gc_dNoTimeout != pSharedChain->m_dTimeout)
        {
            m_pLabor->GetDispatcher()->AddEvent(timer_watcher, ChainTimeoutCallback, pSharedChain->m_dTimeout);
        }
        return(true);
    }
    else
    {
        LOG4_ERROR("chain %u exist.", pSharedChain->GetSequence());
    }
    return(false);
}

bool ActorBuilder::RegisterActor(const std::string& strActorName, Actor* pNewActor, Actor* pCreator)
{
    if (nullptr == pNewActor)
    {
        return(false);
    }
    pNewActor->SetLabor(m_pLabor);
    pNewActor->SetActiveTime(m_pLabor->GetNowTime());
    pNewActor->SetActorName(strActorName);
    pNewActor->SetActorStatus(Actor::ACT_STATUS_ACTIVITY);
    if (nullptr != pCreator && pNewActor->GetActorType() != Actor::ACT_CONTEXT)
    {
        pNewActor->SetContext(pCreator->GetContext());
    }
    return(true);
}

bool ActorBuilder::SendToCluster(const std::string& strIdentify, bool bWithSsl, bool bPipeline, const RedisMsg& oRedisMsg, uint32 uiStepSeq, bool bEnableReadOnly)
{
    bool bSendResult = false;
    auto iter = m_mapClusterChannelStep.find(strIdentify);
    if (iter == m_mapClusterChannelStep.end())
    {
        auto pSharedStep = MakeSharedStep(nullptr, "neb::StepRedisCluster", strIdentify, (bool)bWithSsl, (bool)bPipeline, (bool)bEnableReadOnly);
        if (pSharedStep != nullptr)
        {
            m_mapClusterChannelStep.insert(std::make_pair(strIdentify, pSharedStep));
            bSendResult = pSharedStep->SendTo(strIdentify, oRedisMsg, bWithSsl, bPipeline, uiStepSeq);
        }
    }
    else
    {
        bSendResult = iter->second->SendTo(strIdentify, oRedisMsg, bWithSsl, bPipeline, uiStepSeq);
    }
    return(bSendResult);
}

std::shared_ptr<Session> ActorBuilder::GetSession(uint32 uiSessionId)
{
    std::ostringstream oss;
    oss << uiSessionId;
    auto id_iter = m_mapCallbackSession.find(oss.str());
    if (id_iter == m_mapCallbackSession.end())
    {
        return(nullptr);
    }
    else
    {
        id_iter->second->SetActiveTime(m_pLabor->GetNowTime());
        return(id_iter->second);
    }
}

std::shared_ptr<Session> ActorBuilder::GetSession(const std::string& strSessionId)
{
    auto id_iter = m_mapCallbackSession.find(strSessionId);
    if (id_iter == m_mapCallbackSession.end())
    {
        return(nullptr);
    }
    else
    {
        id_iter->second->SetActiveTime(m_pLabor->GetNowTime());
        return(id_iter->second);
    }
}

bool ActorBuilder::ExecStep(uint32 uiStepSeq, int iErrno, const std::string& strErrMsg, void* data)
{
    auto iter = m_mapCallbackStep.find(uiStepSeq);
    if (iter == m_mapCallbackStep.end())
    {
        return(false);
    }
    else
    {
        iter->second->Emit(iErrno, strErrMsg, data);
        return(true);
    }
}

std::shared_ptr<Operator> ActorBuilder::GetOperator(const std::string& strOperatorName)
{
    auto iter = m_mapOperator.find(strOperatorName);
    if (iter == m_mapOperator.end())
    {
        return(nullptr);
    }
    else
    {
        return(iter->second);
    }
}

bool ActorBuilder::ResetTimeout(std::shared_ptr<Actor> pSharedActor)
{
    ev_timer* watcher = pSharedActor->MutableWatcher()->MutableTimerWatcher();
    ev_tstamp after = m_pLabor->GetNowTime() + pSharedActor->GetTimeout();
    m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
    return(true);
}

int32 ActorBuilder::GetStepNum()
{
    return((int32)m_mapCallbackStep.size());
}

std::shared_ptr<NetLogger> ActorBuilder::GetLogger() const
{
    return(m_pLogger);
}

bool ActorBuilder::ReloadCmdConf()
{
    for (auto cmd_iter = m_mapCmd.begin(); cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        cmd_iter->second->Init();
    }
    for (auto module_iter = m_mapModule.begin(); module_iter != m_mapModule.end(); ++module_iter)
    {
        module_iter->second->Init();
    }
    return(true);
}

bool ActorBuilder::AddNetLogMsg(const MsgBody& oMsgBody)
{
    // 此函数不能写日志，不然可能会导致写日志函数与此函数无限递归
    m_pSessionLogger->AddMsg(oMsgBody);
    return(true);
}

void ActorBuilder::BootLoadCmd(CJsonObject& oCmdConf)
{
    LOG4_TRACE(" ");
    int32 iCmd = 0;
    std::string strUrlPath;
    for (int i = 0; i < oCmdConf["cmd"].GetArraySize(); ++i)
    {
        oCmdConf["cmd"][i].Get("cmd", iCmd);
        auto pCmd = MakeSharedCmd(nullptr, oCmdConf["cmd"][i]("class"), (int32)iCmd);
        if (pCmd != nullptr)
        {
            LOG4_INFO("succeed in loading %s with cmd %d", oCmdConf["cmd"][i]("class").c_str(), iCmd);
        }
    }
    for (int j = 0; j < oCmdConf["module"].GetArraySize(); ++j)
    {
        oCmdConf["module"][j].Get("path", strUrlPath);
        auto pModule = MakeSharedModule(nullptr, oCmdConf["module"][j]("class"), strUrlPath);
        if (pModule != nullptr)
        {
            LOG4_INFO("succeed in loading %s with module %s", oCmdConf["module"][j]("class").c_str(), strUrlPath.c_str());
        }
    }
}

void ActorBuilder::DynamicLoad(CJsonObject& oDynamicLoadingConf)
{
    LOG4_TRACE(" ");
    std::string strVersion = "1.0";
    bool bIsload = false;
    std::string strSoPath;
    std::unordered_map<std::string, tagSo*>::iterator so_iter;
    tagSo* pSo = nullptr;
    for (int i = 0; i < oDynamicLoadingConf.GetArraySize(); ++i)
    {
        oDynamicLoadingConf[i].Get("load", bIsload);
        if (bIsload)
        {
            std::string strSoFile = m_pLabor->GetNodeInfo().strWorkPath + std::string("/")
                    + oDynamicLoadingConf[i]("so_path") + std::string(".") + oDynamicLoadingConf[i]("version");
            if (m_pLabor->GetNodeInfo().bThreadMode && Labor::LABOR_MANAGER != m_pLabor->GetLaborType())
            {
                LoadDynamicSymbol(oDynamicLoadingConf[i]);
            }
            else
            {
                if (0 != access(strSoFile.c_str(), F_OK))
                {
                    strSoFile = m_pLabor->GetNodeInfo().strWorkPath + std::string("/") + oDynamicLoadingConf[i]("so_path");
                    if (0 != access(strSoFile.c_str(), F_OK))
                    {
                        LOG4_WARNING("%s not exist!", strSoFile.c_str());
                        continue;
                    }
                }
                pSo = LoadSo(strSoFile, strVersion);
                if (pSo != nullptr)
                {
                    LOG4_INFO("succeed in loading %s", strSoFile.c_str());
                    LoadDynamicSymbol(oDynamicLoadingConf[i]);
                }
            }
        }
    }
}

ActorBuilder::tagSo* ActorBuilder::LoadSo(const std::string& strSoPath, const std::string& strVersion)
{
    LOG4_TRACE(" ");
    tagSo* pSo = new tagSo();
    if (NULL == pSo)
    {
        return(nullptr);
    }
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        if (pHandle != NULL)
        {
            dlclose(pHandle);
        }
        delete pSo;
        return(nullptr);
    }
    pSo->pSoHandle = pHandle;
    pSo->strVersion = strVersion;
    return(pSo);
}

void ActorBuilder::LoadDynamicSymbol(CJsonObject& oOneSoConf)
{
    int32 iCmd = 0;
    std::string strUrlPath;
    for (int i = 0; i < oOneSoConf["cmd"].GetArraySize(); ++i)
    {
        std::unordered_set<int32> setCmd;
        oOneSoConf["cmd"][i].Get("cmd", iCmd);
        MakeSharedCmd(nullptr, oOneSoConf["cmd"][i]("class"), (int32)iCmd);
    }
    for (int j = 0; j < oOneSoConf["module"].GetArraySize(); ++j)
    {
        std::unordered_set<std::string> setModule;
        oOneSoConf["module"][j].Get("path", strUrlPath);
        MakeSharedModule(nullptr, oOneSoConf["module"][j]("class"), strUrlPath);
    }
}

} /* namespace neb */
