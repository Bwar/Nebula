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
#include "step/Step.hpp"
#include "model/Model.hpp"
#include "chain/Chain.hpp"
#include "actor/session/sys_session/SessionLogger.hpp"
#include "ios/Dispatcher.hpp"
#include "channel/SocketChannel.hpp"

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

    for (auto so_iter = m_mapLoadedSo.begin();
                    so_iter != m_mapLoadedSo.end(); ++so_iter)
    {
        DELETE(so_iter->second);
    }
    m_mapLoadedSo.clear();
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
    return(true);
}

void ActorBuilder::StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        pStep->m_pLabor->GetActorBuilder()->OnStepTimeout(std::dynamic_pointer_cast<Step>(pStep->shared_from_this()));
    }
}

void ActorBuilder::SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        pSession->m_pLabor->GetActorBuilder()->OnSessionTimeout(std::dynamic_pointer_cast<Session>(pSession->shared_from_this()));
    }
}

void ActorBuilder::ChainTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Chain* pChain = (Chain*)watcher->data;
        pChain->m_pLabor->GetActorBuilder()->OnChainTimeout(std::dynamic_pointer_cast<Chain>(pChain->shared_from_this()));
    }
}

bool ActorBuilder::OnStepTimeout(std::shared_ptr<Step> pStep)
{
    ev_timer* watcher = pStep->MutableTimerWatcher();
    ev_tstamp after = pStep->GetActiveTime() - m_pLabor->GetNowTime() + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
        m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
        return(true);
    }
    else    // 步骤已超时
    {
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
}

bool ActorBuilder::OnSessionTimeout(std::shared_ptr<Session> pSession)
{
    ev_timer* watcher = pSession->MutableTimerWatcher();
    LOG4_TRACE("CHECK watchar = 0x%x", watcher);
    ev_tstamp after = pSession->GetActiveTime() - m_pLabor->GetNowTime() + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("session_id: %s", pSession->GetSessionId().c_str());
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
    ev_timer* watcher = pChain->MutableTimerWatcher();
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
                        oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                        oOutMsgHead.set_seq(oMsgHead.seq());
                        oOutMsgHead.set_len(oOutMsgBody.ByteSize());
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
                        oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                        oOutMsgHead.set_seq(oMsgHead.seq());
                        oOutMsgHead.set_len(oOutMsgBody.ByteSize());
                    }
                }
//#else
                snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                LOG4_ERROR(m_pErrBuff);
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                oOutMsgHead.set_seq(oMsgHead.seq());
                oOutMsgHead.set_len(oOutMsgBody.ByteSize());
//#endif
            }
        }
        if (CMD_RSP_SYS_ERROR == oOutMsgHead.cmd())
        {
            LOG4_TRACE("no cmd handler.");
            m_pLabor->GetDispatcher()->SendTo(pChannel, oOutMsgHead.cmd(), oOutMsgHead.seq(), oOutMsgBody);
            return(false);
        }
    }
    else    // 回调
    {
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
                if (CMD_STATUS_RUNNING != eResult && CMD_STATUS_FAULT != eResult)
                {
                    uint32 uiChainId = step_iter->second->GetChainId();
                    RemoveStep(step_iter->second);
                    if (0 != uiChainId)
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
            ExecAssemblyLine(pChannel, oMsgHead, oMsgBody);
        }
        else
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %u had been timeout!", oMsgHead.seq());
            LOG4_WARNING(m_pErrBuff);
            return(false);
        }
    }
    return(true);
}

bool ActorBuilder::OnMessage(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    LOG4_DEBUG("oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    oHttpMsg.type(), oHttpMsg.path().c_str());
    if (HTTP_REQUEST == oHttpMsg.type())    // 新请求
    {
        auto module_iter = m_mapModule.find(oHttpMsg.path());
        if (module_iter == m_mapModule.end())
        {
            module_iter = m_mapModule.find("/switch");
            if (module_iter == m_mapModule.end())
            {
                module_iter = m_mapModule.find("/route");
                if (module_iter == m_mapModule.end())
                {
                    HttpMsg oOutHttpMsg;
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no module to dispose %s!", oHttpMsg.path().c_str());
                    LOG4_ERROR(m_pErrBuff);
                    oOutHttpMsg.set_type(HTTP_RESPONSE);
                    oOutHttpMsg.set_status_code(404);
                    oOutHttpMsg.set_http_major(oHttpMsg.http_major());
                    oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
                    m_pLabor->GetDispatcher()->SendTo(pChannel, oOutHttpMsg, 0);
                }
                else
                {
                    std::ostringstream oss;
                    oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
                    module_iter->second->SetTraceId(oss.str());
                    module_iter->second->AnyMessage(pChannel, oHttpMsg);
                }
            }
            else
            {
                std::ostringstream oss;
                oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
                module_iter->second->SetTraceId(oss.str());
                module_iter->second->AnyMessage(pChannel, oHttpMsg);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << m_pLabor->GetNodeInfo().uiNodeId << "." << m_pLabor->GetNowTime() << "." << m_pLabor->GetSequence();
            module_iter->second->SetTraceId(oss.str());
            module_iter->second->AnyMessage(pChannel, oHttpMsg);
        }
    }
    else
    {
        auto http_step_iter = m_mapCallbackStep.find(pChannel->GetStepSeq());
        if (http_step_iter == m_mapCallbackStep.end())
        {
            LOG4_TRACE("no callback for http response from %s!", oHttpMsg.url().c_str());
            m_pLabor->GetDispatcher()->SetChannelStatus(pChannel, CHANNEL_STATUS_ESTABLISHED, 0);
            m_pLabor->GetDispatcher()->AddNamedSocketChannel(pChannel->GetIdentify(), pChannel);       // push back to named socket channel pool.
        }
        else
        {
            E_CMD_STATUS eResult;
            http_step_iter->second->SetActiveTime(m_pLabor->GetNowTime());
            eResult = (std::dynamic_pointer_cast<HttpStep>(http_step_iter->second))->Callback(pChannel, oHttpMsg);
            if (CMD_STATUS_RUNNING != eResult && CMD_STATUS_FAULT != eResult)
            {
                uint32 uiChainId = http_step_iter->second->GetChainId();
                RemoveStep(http_step_iter->second);
                m_pLabor->GetDispatcher()->SetChannelStatus(pChannel, CHANNEL_STATUS_ESTABLISHED, 0);
                m_pLabor->GetDispatcher()->AddNamedSocketChannel(pChannel->GetIdentify(), pChannel);       // push back to named socket channel pool.
                if (0 != uiChainId)
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
    return(true);
}

bool ActorBuilder::OnRedisConnected(std::shared_ptr<RedisChannel> pChannel, const redisAsyncContext *c, int status)
{
    if (status == REDIS_OK)
    {
        pChannel->bIsReady = true;
        int iCmdStatus;
        for (auto step_iter = pChannel->listPipelineStep.begin();
                        step_iter != pChannel->listPipelineStep.end(); )
        {
            size_t args_size = (*step_iter)->GetCmdArguments().size() + 1;
            const char* argv[args_size];
            size_t arglen[args_size];
            argv[0] = (*step_iter)->GetCmd().c_str();
            arglen[0] = (*step_iter)->GetCmd().size();
            std::vector<std::pair<std::string, bool> >::const_iterator c_iter = (*step_iter)->GetCmdArguments().begin();
            for (size_t i = 1; c_iter != (*step_iter)->GetCmdArguments().end(); ++c_iter, ++i)
            {
                argv[i] = c_iter->first.c_str();
                arglen[i] = c_iter->first.size();
            }
            iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, Dispatcher::RedisCmdCallback, nullptr, args_size, argv, arglen);
            if (iCmdStatus == REDIS_OK)
            {
                LOG4_TRACE("succeed in sending redis cmd: %s", (*step_iter)->CmdToString().c_str());
                ++step_iter;
            }
            else
            {
                E_CMD_STATUS eResult;
                uint32 uiChainId;
                auto interrupt_step_iter = step_iter;
                for (; step_iter != pChannel->listPipelineStep.end(); ++step_iter)
                {
                    eResult = (*step_iter)->Callback(c, status, nullptr);
                    uiChainId = (*step_iter)->GetChainId();
                    RemoveStep(*step_iter);
                    if (CMD_STATUS_RUNNING != eResult && CMD_STATUS_FAULT != eResult)
                    {
                        if (0 != uiChainId)
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
                    else
                    {
                        RemoveChain(uiChainId);
                    }
                }
                for (auto erase_step_iter = interrupt_step_iter;
                               erase_step_iter != pChannel->listPipelineStep.end();)
                {
                    RemoveChain((*erase_step_iter)->GetChainId());
                }
                pChannel->listPipelineStep.clear();
                return(false);
            }
        }
    }
    else
    {
        for (auto step_iter = pChannel->listPipelineStep.begin();
                        step_iter != pChannel->listPipelineStep.end(); ++step_iter)
        {
            (*step_iter)->Callback(c, status, nullptr);
            RemoveChain((*step_iter)->GetChainId());
            RemoveStep(*step_iter);
        }
        pChannel->listPipelineStep.clear();
        return(false);
    }
    return(true);
}

void ActorBuilder::OnRedisDisconnected(std::shared_ptr<RedisChannel> pChannel, const redisAsyncContext *c, int status)
{
    for (auto step_iter = pChannel->listPipelineStep.begin();
                    step_iter != pChannel->listPipelineStep.end(); ++step_iter)
    {
        LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                        c->err, (*step_iter)->CmdToString().c_str());
        (*step_iter)->Callback(c, c->err, nullptr);
        RemoveChain((*step_iter)->GetChainId());
        RemoveStep(std::dynamic_pointer_cast<Step>(*step_iter));
    }
    pChannel->listPipelineStep.clear();
}

bool ActorBuilder::OnRedisCmdResult(std::shared_ptr<RedisChannel> pChannel, redisAsyncContext *c, void *reply, void *privdata)
{
    if (pChannel->listPipelineStep.empty())
    {
        LOG4_ERROR("no redis step!");
        return(false);
    }

    auto step_iter = pChannel->listPipelineStep.begin();
    if (nullptr == reply)
    {
        LOG4_ERROR("redis %s error %d: %s", pChannel->GetIdentify().c_str(), c->err, c->errstr);
        for ( ; step_iter != pChannel->listPipelineStep.end(); ++step_iter)
        {
            LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->CmdToString().c_str());
            (*step_iter)->Callback(c, c->err, (redisReply*)reply);
            RemoveChain((*step_iter)->GetChainId());
            RemoveStep(*step_iter);
        }
        pChannel->listPipelineStep.clear();
    }
    else
    {
        if (step_iter != pChannel->listPipelineStep.end())
        {
            LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->CmdToString().c_str());
            E_CMD_STATUS eResult = (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply);
            pChannel->listPipelineStep.erase(step_iter);
            if (CMD_STATUS_RUNNING != eResult && CMD_STATUS_FAULT != eResult)
            {
                uint32 uiChainId = (*step_iter)->GetChainId();
                if (0 != uiChainId)
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
            //freeReplyObject(reply);
        }
        else
        {
            LOG4_ERROR("no redis callback data found!");
        }
    }

    RemoveStep(std::dynamic_pointer_cast<Step>(*step_iter));
    return(true);
}

void ActorBuilder::AddAssemblyLine(std::shared_ptr<Session> pSession)
{
    m_setAssemblyLine.insert(pSession);
}

void ActorBuilder::RemoveStep(std::shared_ptr<Step> pStep)
{
    if (nullptr == pStep)
    {
        return;
    }
    std::unordered_map<uint32, std::shared_ptr<Step> >::iterator callback_iter;
    for (auto step_seq_iter = pStep->m_setPreStepSeq.begin();
                    step_seq_iter != pStep->m_setPreStepSeq.end(); )
    {
        callback_iter = m_mapCallbackStep.find(*step_seq_iter);
        if (callback_iter == m_mapCallbackStep.end())
        {
            pStep->m_setPreStepSeq.erase(step_seq_iter++);
        }
        else
        {
            LOG4_DEBUG("step %u had pre step %u running, delay delete callback.", pStep->GetSequence(), *step_seq_iter);
            ResetTimeout(std::dynamic_pointer_cast<Actor>(pStep));
            return;
        }
    }
    auto class_iter = m_mapLoadedStep.find(pStep->GetActorName());
    if (class_iter != m_mapLoadedStep.end())
    {
        auto id_iter = class_iter->second.find(pStep->GetSequence());
        if (id_iter != class_iter->second.end())
        {
            class_iter->second.erase(id_iter);
        }
    }
    m_pLabor->GetDispatcher()->DelEvent(pStep->MutableTimerWatcher());
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
    auto class_iter = m_mapLoadedSession.find(pSession->GetActorName());
    if (class_iter != m_mapLoadedSession.end())
    {
        auto id_iter = class_iter->second.find(pSession->GetSessionId());
        if (id_iter != class_iter->second.end())
        {
            class_iter->second.erase(id_iter);
        }
    }
    m_pLabor->GetDispatcher()->DelEvent(pSession->MutableTimerWatcher());
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
        m_pLabor->GetDispatcher()->DelEvent(pChain->MutableTimerWatcher());
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

void ActorBuilder::ExecAssemblyLine(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    for (auto session_iter = m_setAssemblyLine.begin(); session_iter != m_setAssemblyLine.end(); ++session_iter)
    {
        uint32 uiStepSeq = 0;
        do
        {
            uiStepSeq = (*session_iter)->PopWaitingStep();
            auto step_iter = m_mapCallbackStep.find(uiStepSeq);
            if (step_iter != m_mapCallbackStep.end())
            {
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(m_pLabor->GetNowTime());
                eResult = (std::dynamic_pointer_cast<PbStep>(step_iter->second))->Callback(pChannel, oMsgHead, oMsgBody);
                if (CMD_STATUS_RUNNING != eResult && CMD_STATUS_FAULT != eResult)
                {
                    uint32 uiChainId = step_iter->second->GetChainId();
                    RemoveStep(step_iter->second);
                    if (0 != uiChainId)
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
        while(uiStepSeq > 0);
    }
    m_setAssemblyLine.clear();
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
        MakeSharedCmd(nullptr, "neb::CmdDataReport", (int)CMD_REQ_DATA_REPORT);
    }
    else
    {
        MakeSharedCmd(nullptr, "neb::CmdToldWorker", (int)CMD_REQ_TELL_WORKER);
        MakeSharedCmd(nullptr, "neb::CmdUpdateNodeId", (int)CMD_REQ_REFRESH_NODE_ID);
        MakeSharedCmd(nullptr, "neb::CmdNodeNotice", (int)CMD_REQ_NODE_NOTICE);
        MakeSharedCmd(nullptr, "neb::CmdSetNodeConf", (int)CMD_REQ_SET_NODE_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdSetNodeCustomConf", (int)CMD_REQ_SET_NODE_CUSTOM_CONFIG);
        MakeSharedCmd(nullptr, "neb::CmdReloadCustomConf", (int)CMD_REQ_RELOAD_CUSTOM_CONFIG);
    }
    m_pSessionLogger = std::dynamic_pointer_cast<SessionLogger>(MakeSharedSession(nullptr, "neb::SessionLogger"));
}

std::shared_ptr<Actor> ActorBuilder::InitializeSharedActor(Actor* pCreator, std::shared_ptr<Actor> pSharedActor, const std::string& strActorName)
{
    pSharedActor->SetLabor(m_pLabor);
    pSharedActor->SetActiveTime(m_pLabor->GetNowTime());
    pSharedActor->SetActorName(strActorName);
    if (nullptr != pCreator && pSharedActor->GetActorType() != Actor::ACT_CONTEXT)
    {
        pSharedActor->SetContext(pCreator->GetContext());
    }
    switch (pSharedActor->GetActorType())
    {
        case Actor::ACT_PB_STEP:
        case Actor::ACT_HTTP_STEP:
        case Actor::ACT_REDIS_STEP:
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
        case Actor::ACT_MODEL:
            if (TransformToSharedModel(pCreator, pSharedActor))
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
            LOG4_ERROR("\"%s\" must be a Step, a Session, a Model, a Cmd or a Module.",
                    strActorName.c_str());
            return(nullptr);
    }
    return(nullptr);
}

bool ActorBuilder::TransformToSharedStep(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    pSharedActor->m_dTimeout = (gc_dDefaultTimeout == pSharedActor->m_dTimeout)
            ? m_pLabor->GetNodeInfo().dStepTimeout : pSharedActor->m_dTimeout;
    ev_timer* timer_watcher = pSharedActor->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }

    if (nullptr != pCreator)
    {
        pSharedActor->SetTraceId(pCreator->GetTraceId());
    }

    std::shared_ptr<Step> pSharedStep = std::dynamic_pointer_cast<Step>(pSharedActor);
    for (auto iter = pSharedStep->m_setNextStepSeq.begin(); iter != pSharedStep->m_setNextStepSeq.end(); ++iter)
    {
        auto callback_iter = m_mapCallbackStep.find(*iter);
        if (callback_iter != m_mapCallbackStep.end())
        {
            callback_iter->second->m_setPreStepSeq.insert(pSharedStep->GetSequence());
        }
    }

    auto ret = m_mapCallbackStep.insert(std::make_pair(pSharedStep->GetSequence(), pSharedStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pSharedStep->m_dTimeout)
        {
            m_pLabor->GetDispatcher()->AddEvent(timer_watcher, StepTimeoutCallback, pSharedStep->m_dTimeout);
        }
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSharedStep->GetSequence(), pSharedStep->GetActiveTime(), pSharedStep->GetTimeout());
        auto step_class_iter = m_mapLoadedStep.find(pSharedStep->GetActorName());
        if (step_class_iter != m_mapLoadedStep.end())
        {
            step_class_iter->second.insert(pSharedStep->GetSequence());
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

bool ActorBuilder::TransformToSharedSession(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    ev_timer* timer_watcher = pSharedActor->MutableTimerWatcher();
    if (NULL == timer_watcher)
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
        auto session_class_iter = m_mapLoadedSession.find(pSharedSession->GetActorName());
        if (session_class_iter != m_mapLoadedSession.end())
        {
            session_class_iter->second.insert(pSharedSession->GetSessionId());
        }
        return(true);
    }
    else
    {
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
            auto cmd_class_iter = m_mapLoadedCmd.find(pSharedCmd->GetActorName());
            if (cmd_class_iter != m_mapLoadedCmd.end())
            {
                cmd_class_iter->second.insert(pSharedCmd->GetCmd());
            }
            return(true);
        }
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
            auto module_class_iter = m_mapLoadedModule.find(pSharedModule->GetActorName());
            if (module_class_iter != m_mapLoadedModule.end())
            {
                module_class_iter->second.insert(pSharedModule->GetModulePath());
            }
            return(true);
        }
    }
    return(false);
}

bool ActorBuilder::TransformToSharedModel(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    std::shared_ptr<Model> pSharedModel = std::dynamic_pointer_cast<Model>(pSharedActor);
    auto ret = m_mapModel.insert(std::make_pair(pSharedModel->GetActorName(), pSharedModel));
    if (ret.second)
    {
        if (pSharedModel->Init())
        {
            return(true);
        }
    }
    return(false);
}

bool ActorBuilder::TransformToSharedChain(Actor* pCreator, std::shared_ptr<Actor> pSharedActor)
{
    ev_timer* timer_watcher = pSharedActor->MutableTimerWatcher();
    if (NULL == timer_watcher)
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
    return(false);
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

std::shared_ptr<Model> ActorBuilder::GetModel(const std::string& strModelName)
{
    auto iter = m_mapModel.find(strModelName);
    if (iter == m_mapModel.end())
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
    ev_timer* watcher = pSharedActor->MutableTimerWatcher();
    ev_tstamp after = m_pLabor->GetNowTime() + pSharedActor->GetTimeout();
    m_pLabor->GetDispatcher()->RefreshEvent(watcher, after);
    return(true);
}

int32 ActorBuilder::GetStepNum()
{
    return((int32)m_mapCallbackStep.size());
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
        MakeSharedCmd(nullptr, oCmdConf["cmd"][i]("class"), iCmd);
    }
    for (int j = 0; j < oCmdConf["module"].GetArraySize(); ++j)
    {
        oCmdConf["module"][j].Get("path", strUrlPath);
        MakeSharedModule(nullptr, oCmdConf["module"][j]("class"), strUrlPath);
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
            if (oDynamicLoadingConf[i].Get("so_path", strSoPath) && oDynamicLoadingConf[i].Get("version", strVersion))
            {
                so_iter = m_mapLoadedSo.find(strSoPath);
                if (so_iter == m_mapLoadedSo.end())
                {
                    std::string strSoFile = m_pLabor->GetNodeInfo().strWorkPath + std::string("/")
                        + oDynamicLoadingConf[i]("so_path") + std::string(".") + oDynamicLoadingConf[i]("version");
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
                        m_mapLoadedSo.insert(std::make_pair(strSoPath, pSo));
                        LoadDynamicSymbol(oDynamicLoadingConf[i]);
                    }
                }
                else
                {
                    if (strVersion != so_iter->second->strVersion)  // 版本升级，先卸载再加载
                    {
                        std::string strSoFile = m_pLabor->GetNodeInfo().strWorkPath + std::string("/")
                            + oDynamicLoadingConf[i]("so_path") + std::string(".") + oDynamicLoadingConf[i]("version");
                        if (0 != access(strSoFile.c_str(), F_OK))
                        {
                            strSoFile = m_pLabor->GetNodeInfo().strWorkPath + std::string("/") + oDynamicLoadingConf[i]("so_path");
                            if (0 != access(strSoFile.c_str(), F_OK))
                            {
                                LOG4_WARNING("%s not exist!", strSoFile.c_str());
                                continue;
                            }
                        }
                        UnloadDynamicSymbol(oDynamicLoadingConf[i]);
                        dlclose(so_iter->second->pSoHandle);
                        delete so_iter->second;
                        pSo = LoadSo(strSoFile, strVersion);
                        if (pSo != nullptr)
                        {
                            LOG4_INFO("succeed in loading %s", strSoFile.c_str());
                            so_iter->second = pSo;
                            LoadDynamicSymbol(oDynamicLoadingConf[i]);
                        }
                        else
                        {
                            m_mapLoadedSo.erase(so_iter);
                        }
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oDynamicLoadingConf[i].Get("so_path", strSoPath))
            {
                so_iter = m_mapLoadedSo.find(strSoPath);
                if (so_iter != m_mapLoadedSo.end())
                {
                    UnloadDynamicSymbol(oDynamicLoadingConf[i]);
                    dlclose(so_iter->second->pSoHandle);
                    delete so_iter->second;
                    m_mapLoadedSo.erase(so_iter);
                    LOG4_INFO("unload %s.%s", strSoPath.c_str(),
                            oDynamicLoadingConf[i]("version").c_str());
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
        m_mapLoadedCmd.insert(std::make_pair(oOneSoConf["cmd"][i]("class"), setCmd));
        oOneSoConf["cmd"][i].Get("cmd", iCmd);
        MakeSharedCmd(nullptr, oOneSoConf["cmd"][i]("class"), iCmd);
    }
    for (int j = 0; j < oOneSoConf["module"].GetArraySize(); ++j)
    {
        std::unordered_set<std::string> setModule;
        m_mapLoadedModule.insert(std::make_pair(oOneSoConf["module"][j]("class"), setModule));
        oOneSoConf["module"][j].Get("path", strUrlPath);
        MakeSharedModule(nullptr, oOneSoConf["module"][j]("class"), strUrlPath);
    }
    for (int k = 0; k < oOneSoConf["session"].GetArraySize(); ++k)
    {
        std::unordered_set<std::string> setSession;
        m_mapLoadedSession.insert(std::make_pair(oOneSoConf["session"](k), setSession));
    }
    for (int l = 0; l < oOneSoConf["step"].GetArraySize(); ++l)
    {
        std::unordered_set<uint32> setStep;
        m_mapLoadedStep.insert(std::make_pair(oOneSoConf["step"](l), setStep));
    }
}

void ActorBuilder::UnloadDynamicSymbol(CJsonObject& oOneSoConf)
{
    for (int i = 0; i < oOneSoConf["cmd"].GetArraySize(); ++i)
    {
        auto class_iter = m_mapLoadedCmd.find(oOneSoConf["cmd"][i]("class"));
        if (class_iter != m_mapLoadedCmd.end())
        {
            for (auto id_iter = class_iter->second.begin(); id_iter != class_iter->second.end(); ++id_iter)
            {
                auto cmd_iter = m_mapCmd.find(*id_iter);
                if (cmd_iter != m_mapCmd.end())
                {
                    m_mapCmd.erase(cmd_iter);
                }
            }
            class_iter->second.clear();
            m_mapLoadedCmd.erase(class_iter);
        }
    }
    for (int j = 0; j < oOneSoConf["module"].GetArraySize(); ++j)
    {
        auto class_iter = m_mapLoadedModule.find(oOneSoConf["module"][j]("class"));
        if (class_iter != m_mapLoadedModule.end())
        {
            for (auto id_iter = class_iter->second.begin(); id_iter != class_iter->second.end(); ++id_iter)
            {
                auto module_iter = m_mapModule.find(*id_iter);
                if (module_iter != m_mapModule.end())
                {
                    m_mapModule.erase(module_iter);
                }
            }
            class_iter->second.clear();
            m_mapLoadedModule.erase(class_iter);
        }
    }
    for (int k = 0; k < oOneSoConf["session"].GetArraySize(); ++k)
    {
        auto class_iter = m_mapLoadedSession.find(oOneSoConf["session"](k));
        if (class_iter != m_mapLoadedSession.end())
        {
            for (auto id_iter = class_iter->second.begin(); id_iter != class_iter->second.end(); ++id_iter)
            {
                auto session_iter = m_mapCallbackSession.find(*id_iter);
                if (session_iter != m_mapCallbackSession.end())
                {
                    m_mapCallbackSession.erase(session_iter);
                }
            }
            class_iter->second.clear();
            m_mapLoadedSession.erase(class_iter);
        }
    }
    for (int l = 0; l < oOneSoConf["step"].GetArraySize(); ++l)
    {
        auto class_iter = m_mapLoadedStep.find(oOneSoConf["step"](l));
        if (class_iter != m_mapLoadedStep.end())
        {
            for (auto id_iter = class_iter->second.begin(); id_iter != class_iter->second.end(); ++id_iter)
            {
                auto step_iter = m_mapCallbackStep.find(*id_iter);
                if (step_iter != m_mapCallbackStep.end())
                {
                    step_iter->second->Timeout();
                    m_mapCallbackStep.erase(step_iter);
                }
            }
            class_iter->second.clear();
            m_mapLoadedStep.erase(class_iter);
        }
        std::unordered_set<uint32> setStep;
        m_mapLoadedStep.insert(std::make_pair(oOneSoConf["step"](l), setStep));
    }
    for (int k = 0; k < oOneSoConf["model"].GetArraySize(); ++k)
    {
        auto class_iter = m_mapModel.find(oOneSoConf["model"](k));
        if (class_iter != m_mapModel.end())
        {
            m_mapModel.erase(class_iter);
        }
    }
}

} /* namespace neb */
