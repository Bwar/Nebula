/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief
 * @author   Bwar
 * @date:    2018年3月27日
 * @note
 * Modify history:
 ******************************************************************************/

#include <algorithm>
#ifdef __cplusplus
extern "C" {
#endif
#include "util/process_helper.h"
#include "util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "labor/WorkerImpl.hpp"
#include "actor/Actor.hpp"
#include "actor/cmd/Cmd.hpp"
#include "actor/cmd/Module.hpp"
#include "actor/session/Session.hpp"
#include "actor/step/HttpStep.hpp"
#include "actor/step/PbStep.hpp"
#include "actor/step/RedisStep.hpp"
#include "actor/step/Step.hpp"

namespace neb
{

void WorkerImpl::TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->m_pImpl->Terminated(watcher);  // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    }
}

void WorkerImpl::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->m_pImpl->CheckParent();
    }
}

void WorkerImpl::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = static_cast<SocketChannel*>(watcher->data);
        Worker* pWorker = (Worker*)pChannel->m_pImpl->m_pLabor;
        if (revents & EV_READ)
        {
            pWorker->m_pImpl->OnIoRead(pChannel->shared_from_this());
        }
        if (revents & EV_WRITE)
        {
            pWorker->m_pImpl->OnIoWrite(pChannel->shared_from_this());
        }
    }
}

void WorkerImpl::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = static_cast<SocketChannel*>(watcher->data);
        Worker* pWorker = (Worker*)(pChannel->m_pImpl->m_pLabor);
        if (pChannel->m_pImpl->GetFd() < 3)      // TODO 这个判断是不得已的做法，需查找fd为0回调到这里的原因
        {
            return;
        }
        pWorker->m_pImpl->OnIoTimeout(pChannel->shared_from_this());
    }
}

void WorkerImpl::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        WorkerImpl* pWorkerImpl = (WorkerImpl*)(watcher->data);
        pWorkerImpl->CheckParent();
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void WorkerImpl::StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        ((Worker*)(pStep->m_pWorker))->m_pImpl->OnStepTimeout(std::dynamic_pointer_cast<Step>(pStep->shared_from_this()));
    }
}

void WorkerImpl::SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((Worker*)pSession->m_pWorker)->m_pImpl->OnSessionTimeout(std::dynamic_pointer_cast<Session>(pSession->shared_from_this()));
    }
}

void WorkerImpl::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->OnRedisConnected(c, status);
    }
}

void WorkerImpl::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->OnRedisDisconnected(c, status);
    }
}

void WorkerImpl::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->OnRedisCmdResult(c, reply, privdata);
    }
}

WorkerImpl::WorkerImpl(Worker* pWorker, const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf)
    : m_pErrBuff(nullptr), m_pWorker(pWorker), m_loop(nullptr), m_pSessionNode(nullptr)
{
    m_stWorkerInfo.iManagerControlFd = iControlFd;
    m_stWorkerInfo.iManagerDataFd = iDataFd;
    m_stWorkerInfo.iWorkerIndex = iWorkerIndex;
    m_stWorkerInfo.iWorkerPid = getpid();
    m_stWorkerInfo.strWorkPath = strWorkPath;
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
    if (!Init(oJsonConf))
    {
        exit(-1);
    }
    m_oWorkerConf = oJsonConf;
}

WorkerImpl::~WorkerImpl()
{
    Destroy();
}

void WorkerImpl::Run()
{
    LOG4_DEBUG("%s:%d", __FILE__, __LINE__);

    if (!CreateEvents())
    {
        Destroy();
        exit(-2);
    }
    LoadSysCmd();
    BootLoadCmd(m_oWorkerConf["boot_load"]);
    DynamicLoadCmd(m_oWorkerConf["dynamic_loading"]);

    ev_run (m_loop, 0);
}

void WorkerImpl::Terminated(struct ev_signal* watcher)
{
    LOG4_TRACE(" ");
    int iSignum = watcher->signum;
    delete watcher;
    //Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    Destroy();
    exit(iSignum);
}

bool WorkerImpl::CheckParent()
{
    LOG4_TRACE(" ");
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_stWorkerInfo.iWorkerIndex);
        Destroy();
        exit(0);
    }
    MsgBody oMsgBody;
    CJsonObject oJsonLoad;
    oJsonLoad.Add("load", int32(m_stWorkerInfo.iConnectionNum + m_mapCallbackStep.size()));
    oJsonLoad.Add("connect", m_stWorkerInfo.iConnectionNum);
    oJsonLoad.Add("recv_num", m_stWorkerInfo.iRecvNum);
    oJsonLoad.Add("recv_byte", m_stWorkerInfo.iRecvByte);
    oJsonLoad.Add("send_num", m_stWorkerInfo.iSendNum);
    oJsonLoad.Add("send_byte", m_stWorkerInfo.iSendByte);
    oJsonLoad.Add("client", m_stWorkerInfo.iClientNum);
    oMsgBody.set_data(oJsonLoad.ToString());
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    m_pManagerControlChannel->m_pImpl->Send(CMD_REQ_UPDATE_WORKER_LOAD, GetSequence(), oMsgBody);
    m_stWorkerInfo.iRecvNum = 0;
    m_stWorkerInfo.iRecvByte = 0;
    m_stWorkerInfo.iSendNum = 0;
    m_stWorkerInfo.iSendByte = 0;
    return(true);
}

bool WorkerImpl::OnIoRead(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d]", pChannel->m_pImpl->GetFd());
    if (pChannel->m_pImpl->GetFd() == m_stWorkerInfo.iManagerDataFd)
    {
        return(FdTransfer());
    }
    else
    {
        return(RecvDataAndHandle(pChannel));
    }
}

bool WorkerImpl::RecvDataAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    E_CODEC_STATUS eCodecStatus;
    if (CODEC_HTTP == pChannel->m_pImpl->GetCodecType())
    {
        for (int i = 0; ; ++i)
        {
            HttpMsg oHttpMsg;
            if (0 == i)
            {
                eCodecStatus = pChannel->m_pImpl->Recv(oHttpMsg);
            }
            else
            {
                eCodecStatus = pChannel->m_pImpl->Fetch(oHttpMsg);
            }

            if (CODEC_STATUS_OK == eCodecStatus)
            {
                Handle(pChannel, oHttpMsg);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        eCodecStatus = pChannel->m_pImpl->Recv(oMsgHead, oMsgBody);
        while (CODEC_STATUS_OK == eCodecStatus)
        {
            if (m_stWorkerInfo.bIsAccess && !pChannel->m_pImpl->IsChannelVerify())
            {
                if (CODEC_NEBULA != pChannel->m_pImpl->GetCodecType() && pChannel->m_pImpl->GetMsgNum() > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                {
                    LOG4_DEBUG("invalid request, please login first!");
                    DiscardSocketChannel(pChannel);
                    return(false);
                }
            }
            Handle(pChannel, oMsgHead, oMsgBody);
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->m_pImpl->Fetch(oMsgHead, oMsgBody);
        }
    }

    if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        return(true);
    }
    else    // 编解码出错或连接关闭或连接中断
    {
        LOG4_DEBUG("codec error or connection closed!");
        DiscardSocketChannel(pChannel);
        return(false);
    }
}

bool WorkerImpl::FdTransfer()
{
    LOG4_TRACE(" ");
    int iAcceptFd = -1;
    int iCodec = 0;
    int iErrno = SocketChannel::RecvChannelFd(m_stWorkerInfo.iManagerDataFd, iAcceptFd, iCodec, m_pLogger);
    if (iErrno != ERR_OK)
    {
        if (iErrno == ERR_CHANNEL_EOF)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d", m_stWorkerInfo.iManagerDataFd, errno);
            Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        int iKeepAlive = 1;
        int iKeepIdle = 60;
        int iKeepInterval = 5;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPCNT");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
        std::shared_ptr<SocketChannel> pChannel = nullptr;
        LOG4_TRACE("fd[%d] transfer successfully.", iAcceptFd);
        if (CODEC_NEBULA != iCodec && m_oWorkerConf["with_ssl"]("config_path").length() > 0)
        {
            pChannel = CreateSocketChannel(iAcceptFd, E_CODEC_TYPE(iCodec), true, true);
        }
        else
        {
            pChannel = CreateSocketChannel(iAcceptFd, E_CODEC_TYPE(iCodec), true, false);
        }
        if (nullptr != pChannel)
        {
            int z;                          /* status return code */
            struct sockaddr_in adr_inet;    /* AF_INET */
            unsigned int len_inet = 16;                   /* length */
            z = getpeername(iAcceptFd, (struct sockaddr *)&adr_inet, &len_inet);
            if (z == 0)
            {
                LOG4_TRACE("set fd %d's remote addr \"%s\"", iAcceptFd, inet_ntoa(adr_inet.sin_addr));
                pChannel->m_pImpl->SetRemoteAddr(inet_ntoa(adr_inet.sin_addr));
            }
            else
            {
                LOG4_ERROR("getpeername error %d", errno);
            }
            AddIoReadEvent(pChannel);
            if (CODEC_NEBULA == iCodec)
            {
                AddIoTimeout(pChannel, m_stWorkerInfo.dIoTimeout);
                std::shared_ptr<Step> pStepTellWorker = MakeSharedStep(nullptr, "neb::StepTellWorker", pChannel);
                if (nullptr == pStepTellWorker)
                {
                    return(false);
                }
                pStepTellWorker->Emit(ERR_OK);
            }
            else
            {
                AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            }
            return(true);
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
            close(iAcceptFd);
        }
    }
    return(false);
}

bool WorkerImpl::OnIoWrite(std::shared_ptr<SocketChannel> pChannel)
{
    if (CHANNEL_STATUS_TRY_CONNECT == pChannel->m_pImpl->GetChannelStatus())  // connect之后的第一个写事件
    {
        if (CODEC_NEBULA == pChannel->m_pImpl->GetCodecType())  // 系统内部Server间通信
        {
            std::shared_ptr<Step> pStepConnectWorker = MakeSharedStep(nullptr, "neb::StepConnectWorker", pChannel, pChannel->m_pImpl->m_unRemoteWorkerIdx);
            if (nullptr == pStepConnectWorker)
            {
                LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
                return(false);
            }
            if (CMD_STATUS_RUNNING != pStepConnectWorker->Emit(ERR_OK))
            {
                Remove(pStepConnectWorker);
            }
            return(true);
        }
    }

    E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send();
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        AddIoWriteEvent(pChannel);
    }
    else
    {
        DiscardSocketChannel(pChannel);
    }
    return(true);
}

bool WorkerImpl::OnIoTimeout(std::shared_ptr<SocketChannel> pChannel)
{
    ev_tstamp after = pChannel->m_pImpl->GetActiveTime() - ev_now(m_loop) + m_stWorkerInfo.dIoTimeout;
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, pChannel->m_pImpl->MutableTimerWatcher());
        ev_timer_set (pChannel->m_pImpl->MutableTimerWatcher(), after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, pChannel->m_pImpl->MutableTimerWatcher());
        return(true);
    }

    LOG4_TRACE("fd %d, seq %u:", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    if (pChannel->m_pImpl->NeedAliveCheck())     // 需要发送心跳检查
    {
        std::shared_ptr<Step> pStepIoTimeout = MakeSharedStep(nullptr, "neb::StepIoTimeout", pChannel);
        if (nullptr == pStepIoTimeout)
        {
            LOG4_ERROR("new StepIoTimeout error!");
            DiscardSocketChannel(pChannel);
        }
        E_CMD_STATUS eStatus = pStepIoTimeout->Emit(ERR_OK);
        if (CMD_STATUS_RUNNING != eStatus)
        {
            // 若返回非running状态，则表明发包时已出错，
            // 销毁连接过程在SendTo里已经完成，这里不需要再销毁连接
            Remove(pStepIoTimeout);
        }
    }
    else        // 关闭文件描述符并清理相关资源
    {
        if (CODEC_NEBULA != pChannel->m_pImpl->GetCodecType())   // 非内部服务器间的连接才会在超时中关闭
        {
            LOG4_TRACE("io timeout!");
            DiscardSocketChannel(pChannel);
        }
    }
    return(true);
}

bool WorkerImpl::OnStepTimeout(std::shared_ptr<Step> pStep)
{
    ev_timer* watcher = pStep->MutableTimerWatcher();
    ev_tstamp after = pStep->GetActiveTime() - ev_now(m_loop) + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 步骤已超时
    {
        LOG4_TRACE("seq %lu: active_time %lf, now_time %lf, lifetime %lf",
                        pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
        E_CMD_STATUS eResult = pStep->Timeout();
        if (CMD_STATUS_RUNNING == eResult)
        {
            ev_tstamp after = pStep->GetTimeout();
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            Remove(pStep);
            return(true);
        }
    }
}

bool WorkerImpl::OnSessionTimeout(std::shared_ptr<Session> pSession)
{
    ev_timer* watcher = pSession->MutableTimerWatcher();
    ev_tstamp after = pSession->GetActiveTime() - ev_now(m_loop) + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("session_id: %s", pSession->GetSessionId().c_str());
        if (CMD_STATUS_RUNNING == pSession->Timeout())
        {
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, pSession->GetTimeout() + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            Remove(pSession);
            return(true);
        }
    }
}

bool WorkerImpl::OnRedisConnected(const redisAsyncContext *c, int status)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        if (status == REDIS_OK)
        {
            channel_iter->second->bIsReady = true;
            int iCmdStatus;
            for (auto step_iter = channel_iter->second->listPipelineStep.begin();
                            step_iter != channel_iter->second->listPipelineStep.end(); )
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
                iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, RedisCmdCallback, nullptr, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", (*step_iter)->CmdToString().c_str());
                }
                else
                {
                    auto interrupt_step_iter = step_iter;
                    for (; step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
                    {
                        (*step_iter)->Callback(c, status, nullptr);
                        Remove(*step_iter);
                    }
                    if (step_iter == channel_iter->second->listPipelineStep.begin())    // 第一个命令就发送失败
                    {
                        channel_iter->second->listPipelineStep.clear();
                    }
                    else
                    {
                        for (auto erase_step_iter = interrupt_step_iter;
                                        erase_step_iter != channel_iter->second->listPipelineStep.end();)
                        {
                            channel_iter->second->listPipelineStep.erase(erase_step_iter++);
                        }
                    }
                    DelNamedRedisChannel(channel_iter->second->GetIdentify());
                    m_mapRedisChannel.erase(channel_iter);
                }
            }
        }
        else
        {
            for (auto step_iter = channel_iter->second->listPipelineStep.begin();
                            step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
            {
                (*step_iter)->Callback(c, status, nullptr);
                Remove(*step_iter);
            }
            channel_iter->second->listPipelineStep.clear();
            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            m_mapRedisChannel.erase(channel_iter);
        }
    }
    return(true);
}

bool WorkerImpl::OnRedisDisconnected(const redisAsyncContext *c, int status)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        for (auto step_iter = channel_iter->second->listPipelineStep.begin();
                        step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->CmdToString().c_str());
            (*step_iter)->Callback(c, c->err, nullptr);
            Remove(std::dynamic_pointer_cast<Step>(*step_iter));
        }
        channel_iter->second->listPipelineStep.clear();

        DelNamedRedisChannel(channel_iter->second->GetIdentify());
        m_mapRedisChannel.erase(channel_iter);
    }
    redisAsyncFree(const_cast<redisAsyncContext*>(c));
    return(true);
}

bool WorkerImpl::OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        if (channel_iter->second->listPipelineStep.empty())
        {
            LOG4_ERROR("no redis step!");
            return(false);
        }

        auto step_iter = channel_iter->second->listPipelineStep.begin();
        if (nullptr == reply)
        {
            LOG4_ERROR("redis %s error %d: %s", channel_iter->second->GetIdentify().c_str(), c->err, c->errstr);
            for ( ; step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->CmdToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
            }
            channel_iter->second->listPipelineStep.clear();

            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            m_mapRedisChannel.erase(channel_iter);
        }
        else
        {
            if (step_iter != channel_iter->second->listPipelineStep.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->CmdToString().c_str());
                (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply);
                channel_iter->second->listPipelineStep.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }

        Remove(std::dynamic_pointer_cast<Step>(*step_iter));
    }

    return(true);
}

time_t WorkerImpl::GetNowTime() const
{
    return((time_t)ev_now(m_loop));
}

bool WorkerImpl::ResetTimeout(std::shared_ptr<Actor> pActor)
{
    ev_timer* watcher = pActor->MutableTimerWatcher();
    ev_tstamp after = ev_now(m_loop) + pActor->GetTimeout();
    ev_timer_stop (m_loop, watcher);
    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, watcher);
    return(true);
}

bool WorkerImpl::SetProcessName(const CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_stWorkerInfo.iWorkerIndex);
    ngx_setproctitle(szProcessName);
    return(true);
}

bool WorkerImpl::Init(CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_stWorkerInfo.iWorkerIndex);
    ngx_setproctitle(szProcessName);
    oJsonConf.Get("io_timeout", m_stWorkerInfo.dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_stWorkerInfo.dStepTimeout))
    {
        m_stWorkerInfo.dStepTimeout = 0.5;
    }
    oJsonConf.Get("node_type", m_stWorkerInfo.strNodeType);
    oJsonConf.Get("host", m_stWorkerInfo.strHostForServer);
    oJsonConf.Get("port", m_stWorkerInfo.iPortForServer);
    m_oCustomConf = oJsonConf["custom"];
    std::ostringstream oss;
    oss << m_stWorkerInfo.strHostForServer << ":" << m_stWorkerInfo.iPortForServer << "." << m_stWorkerInfo.iWorkerIndex;
    m_stWorkerInfo.strWorkerIdentify = std::move(oss.str());

    if (oJsonConf("access_host").size() > 0 && oJsonConf("access_port").size() > 0)
    {
        m_stWorkerInfo.bIsAccess = true;
        oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_stWorkerInfo.dMsgStatInterval);
        oJsonConf["permission"]["uin_permit"].Get("permit_num", m_stWorkerInfo.iMsgPermitNum);
    }
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    if (oJsonConf["with_ssl"]("config_path").length() > 0)
    {
#ifdef WITH_OPENSSL
        if (ERR_OK != SocketChannelSslImpl::SslInit(m_pLogger))
        {
            LOG4_FATAL("SslInit() failed!");
            return(false);
        }
        if (ERR_OK != SocketChannelSslImpl::SslServerCtxCreate(m_pLogger))
        {
            LOG4_FATAL("SslServerCtxCreate() failed!");
            return(false);
        }
        std::string strCertFile = m_stWorkerInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("cert_file");
        std::string strKeyFile = m_stWorkerInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("key_file");
        if (ERR_OK != SocketChannelSslImpl::SslServerCertificate(m_pLogger, strCertFile, strKeyFile))
        {
            LOG4_FATAL("SslServerCertificate() failed!");
            return(false);
        }
#endif
    }
    return(true);
}

bool WorkerImpl::InitLogger(const CJsonObject& oJsonConf)
{
    if (nullptr != m_pLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        int32 iNetLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        oJsonConf.Get("net_log_level", iNetLogLevel);
        m_pLogger->SetLogLevel(iLogLevel);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iLogLevel = 0;
        int32 iNetLogLevel = 0;
        std::string strLogname = m_stWorkerInfo.strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_stWorkerInfo.strWorkerIdentify;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("net_log_level", iNetLogLevel);
        oJsonConf.Get("log_level", iLogLevel);
        m_pLogger = std::make_shared<neb::NetLogger>(strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, m_pWorker);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        LOG4_NOTICE("%s program begin...", getproctitle());
        return(true);
    }
}

bool WorkerImpl::CreateEvents()
{
    m_loop = ev_loop_new(EVFLAG_AUTO);
    if (m_loop == nullptr)
    {
        return(false);
    }

    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* signal_watcher = new ev_signal();
    ev_signal_init (signal_watcher, TerminatedCallback, SIGINT);
    signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, signal_watcher);

    AddPeriodicTaskEvent();

    // 注册网络IO事件
    m_pManagerDataChannel = std::make_shared<SocketChannel>(m_pLogger, m_stWorkerInfo.iManagerDataFd, GetSequence());
    m_pManagerDataChannel->m_pImpl->SetLabor(m_pWorker);
    m_pManagerDataChannel->m_pImpl->Init(CODEC_NEBULA);
    m_pManagerDataChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    m_pManagerControlChannel = std::make_shared<SocketChannel>(m_pLogger, m_stWorkerInfo.iManagerControlFd, GetSequence());
    m_pManagerControlChannel->m_pImpl->SetLabor(m_pWorker);
    m_pManagerControlChannel->m_pImpl->Init(CODEC_NEBULA);
    m_pManagerControlChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    AddIoReadEvent(m_pManagerDataChannel);
    // AddIoTimeout(pChannelData, 60.0);   // TODO 需要优化
    AddIoReadEvent(m_pManagerControlChannel);
    // AddIoTimeout(pChannelControl, 60.0); //TODO 需要优化
    return(true);
}

void WorkerImpl::LoadSysCmd()
{
    // 调用MakeSharedCmd等系列函数必须严格匹配参数类型，类型不符需显式转换，如 (int)CMD_REQ_CONNECT_TO_WORKER
    MakeSharedCmd(nullptr, "neb::CmdToldWorker", (int)CMD_REQ_TELL_WORKER);
    MakeSharedCmd(nullptr, "neb::CmdUpdateNodeId", (int)CMD_REQ_REFRESH_NODE_ID);
    MakeSharedCmd(nullptr, "neb::CmdNodeNotice", (int)CMD_REQ_NODE_REG_NOTICE);
    MakeSharedCmd(nullptr, "neb::CmdBeat", (int)CMD_REQ_BEAT);
#if __cplusplus >= 201401L
    m_pSessionNode = std::make_unique<SessionNode>();
#else
    m_pSessionNode = std::unique_ptr<SessionNode>(new SessionNode());
#endif
}

void WorkerImpl::Destroy()
{
    LOG4_TRACE(" ");

    m_mapCmd.clear();
    m_mapCallbackStep.clear();
    m_mapCallbackSession.clear();
    m_mapSocketChannel.clear();
    m_mapRedisChannel.clear();
    m_mapNamedSocketChannel.clear();
    m_mapNamedRedisChannel.clear();

    for (auto so_iter = m_mapLoadedSo.begin();
                    so_iter != m_mapLoadedSo.end(); ++so_iter)
    {
        DELETE(so_iter->second);
    }
    m_mapLoadedSo.clear();

#ifdef WITH_OPENSSL
    SocketChannelSslImpl::SslFree();
#endif
    // TODO 待补充完整

    if (m_loop != nullptr)
    {
        ev_loop_destroy(m_loop);
        m_loop = nullptr;
    }

    if (m_pErrBuff != nullptr)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = nullptr;
    }
}

bool WorkerImpl::AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        std::list<std::shared_ptr<SocketChannel>> listChannel;
        listChannel.push_back(pChannel);
        m_mapNamedSocketChannel.insert(std::make_pair(strIdentify, listChannel));
    }
    else
    {
        named_iter->second.push_back(pChannel);
    }
    pChannel->m_pImpl->SetIdentify(strIdentify);
    return(true);

}

void WorkerImpl::DelNamedSocketChannel(const std::string& strIdentify)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        ;
    }
    else
    {
        m_mapNamedSocketChannel.erase(named_iter);
    }
}

void WorkerImpl::SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify)
{
    pChannel->m_pImpl->SetIdentify(strIdentify);
}

bool WorkerImpl::AddNamedRedisChannel(const std::string& strIdentify, std::shared_ptr<RedisChannel> pChannel)
{
    auto named_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (named_iter == m_mapNamedRedisChannel.end())
    {
        std::list<std::shared_ptr<RedisChannel> > listChannel;
        listChannel.push_back(pChannel);
        m_mapNamedRedisChannel.insert(std::make_pair(strIdentify, listChannel));
        return(true);
    }
    else
    {
        named_iter->second.push_back(pChannel);
    }
    pChannel->SetIdentify(strIdentify);
    return(true);
}

void WorkerImpl::DelNamedRedisChannel(const std::string& strIdentify)
{
    auto named_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (named_iter == m_mapNamedRedisChannel.end())
    {
        ;
    }
    else
    {
        m_mapNamedRedisChannel.erase(named_iter);
    }
}

void WorkerImpl::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->AddNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType && m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(true);
    }
}

void WorkerImpl::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->DelNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType && !m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(false);
    }
}

bool WorkerImpl::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send();
    if (CODEC_STATUS_OK == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_PAUSE == eStatus)
    {
        AddIoWriteEvent(pChannel);
        return(true);
    }
    return(false);
}

bool WorkerImpl::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    if (nullptr != pSender)
    {
        (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
    }
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
    if (CODEC_STATUS_OK == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_PAUSE == eStatus)
    {
        AddIoWriteEvent(pChannel);
        return(true);
    }
    return(false);
}

bool WorkerImpl::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        return(AutoSend(strIdentify, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        E_CODEC_STATUS eStatus = (*named_iter->second.begin())->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus)
        {
            RemoveIoWriteEvent(*named_iter->second.begin());
            return(true);
        }
        else if (CODEC_STATUS_PAUSE == eStatus)
        {
            AddIoWriteEvent(*named_iter->second.begin());
            return(true);
        }
        return(false);
    }
}

bool WorkerImpl::SendPolling(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool WorkerImpl::SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("nody_type: %s, factor: %d", strNodeType.c_str(), uiFactor);
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool WorkerImpl::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("nody_type: %s", strNodeType.c_str());
    if (oMsgBody.has_req_target())
    {
        if (0 != oMsgBody.req_target().route_id())
        {
            return(SendOriented(strNodeType, oMsgBody.req_target().route_id(), iCmd, uiSeq, oMsgBody, pSender));
        }
        else if (oMsgBody.req_target().route().length() > 0)
        {
            std::string strOnlineNode;
            if (m_pSessionNode->GetNode(strNodeType, oMsgBody.req_target().route(), strOnlineNode))
            {
                if (nullptr != pSender)
                {
                    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
                }
                return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
            }
            else
            {
                LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
                return(false);
            }
        }
        else
        {
            return(SendPolling(strNodeType, iCmd, uiSeq, oMsgBody, pSender));
        }
    }
    else
    {
        LOG4_ERROR("MsgBody is not a request message!");
        return(false);
    }
};

bool WorkerImpl::Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::unordered_set<std::string> setOnlineNodes;
    if (m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        bool bSendResult = false;
        for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
        {
            bSendResult |= SendTo(*node_iter, iCmd, uiSeq, oMsgBody);
        }
        return(bSendResult);
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool WorkerImpl::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(oHttpMsg, uiHttpStepSeq);
    switch (eStatus)
    {
        case CODEC_STATUS_OK:
            return(true);
        case CODEC_STATUS_PAUSE:
            AddIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_EOF:      // http1.0 respone and close
            DiscardSocketChannel(pChannel);
            return(true);
        default:
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

bool WorkerImpl::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("identify: %s", szIdentify);
    auto named_iter = m_mapNamedSocketChannel.find(szIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", szIdentify);
        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
    }
    else
    {
        if (named_iter->second.empty())
        {
            return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
        }
        else
        {
            auto channel_iter = named_iter->second.begin();
            E_CODEC_STATUS eStatus = (*channel_iter)->m_pImpl->Send(oHttpMsg, uiHttpStepSeq);
            if (CODEC_STATUS_OK == eStatus)
            {
                named_iter->second.erase(channel_iter);     // erase from named channel pool, the channel remain in m_mapSocketChannel.               
                return(true);
            }
            else if (CODEC_STATUS_PAUSE == eStatus)
            {
                AddIoWriteEvent(*channel_iter);
                return(true);
            }
            return(false);
        }
    }
}

bool WorkerImpl::SendTo(std::shared_ptr<RedisChannel> pRedisChannel, Actor* pSender)
{
    LOG4_TRACE(" ");
    if (nullptr == pSender)
    {
        LOG4_ERROR("pSender can't be nullptr!");
        return(false);
    }
    if (Actor::ACT_REDIS_STEP != pSender->GetActorType())
    {
        LOG4_ERROR("The actor which send data to the RedisChannel must be a RedisStep.");
        return(false);
    }
    std::shared_ptr<RedisStep> pRedisStep = std::dynamic_pointer_cast<RedisStep>(pSender->shared_from_this());
    if (pRedisChannel->bIsReady)
    {
        int status;
        size_t args_size = pRedisStep->GetCmdArguments().size() + 1;
        const char* argv[args_size];
        size_t arglen[args_size];
        argv[0] = pRedisStep->GetCmd().c_str();
        arglen[0] = pRedisStep->GetCmd().size();
        std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetCmdArguments().begin();
        for (size_t i = 1; c_iter != pRedisStep->GetCmdArguments().end(); ++c_iter, ++i)
        {
            argv[i] = c_iter->first.c_str();
            arglen[i] = c_iter->first.size();
        }
        status = redisAsyncCommandArgv((redisAsyncContext*)pRedisChannel->RedisContext(), RedisCmdCallback, nullptr, args_size, argv, arglen);
        if (status == REDIS_OK)
        {
            LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->CmdToString().c_str());
            pRedisChannel->listPipelineStep.push_back(pRedisStep);
            return(true);
        }
        else
        {
            LOG4_ERROR("redis status %d!", status);
            return(false);
        }
    }
    else
    {
        pRedisChannel->listPipelineStep.push_back(pRedisStep);
        return(true);
    }
}

bool WorkerImpl::SendTo(const std::string& strIdentify, Actor* pSender)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    if (nullptr == pSender)
    {
        LOG4_ERROR("pSender can't be nullptr!");
        return(false);
    }
    auto ctx_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (ctx_iter != m_mapNamedRedisChannel.end())
    {
        return(SendTo(*(ctx_iter->second.begin()), pSender));
    }
    else
    {
        size_t iPosIpPortSeparator = strIdentify.rfind(':');
        if (iPosIpPortSeparator == std::string::npos)
        {
            LOG4_ERROR("invalid node identify \"%s\"", strIdentify.c_str());
            return(false);
        }
        std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
        std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
        int iPort = atoi(strPort.c_str());
        if (iPort == 0)
        {
            LOG4_ERROR("invalid node identify \"%s\"", strIdentify.c_str());
            return(false);
        }
        if (Actor::ACT_REDIS_STEP != pSender->GetActorType())
        {
            LOG4_ERROR("The actor which send data to the RedisChannel must be a RedisStep.");
            return(false);
        }
        std::shared_ptr<RedisStep> pRedisStep = std::dynamic_pointer_cast<RedisStep>(pSender->shared_from_this());
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool WorkerImpl::SendTo(const std::string& strHost, int iPort, Actor* pSender)
{
    LOG4_TRACE("%s, %d", strHost.c_str(), iPort);
    if (nullptr == pSender)
    {
        LOG4_ERROR("pSender can't be nullptr!");
        return(false);
    }
    std::ostringstream oss;
    oss << strHost << ":" << iPort;
    std::string strIdentify = std::move(oss.str());
    auto ctx_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (ctx_iter != m_mapNamedRedisChannel.end())
    {
        return(SendTo(*(ctx_iter->second.begin()), pSender));
    }
    else
    {
        if (Actor::ACT_REDIS_STEP != pSender->GetActorType())
        {
            LOG4_ERROR("The actor which send data to the RedisChannel must be a RedisStep.");
            return(false);
        }
        std::shared_ptr<RedisStep> pRedisStep = std::dynamic_pointer_cast<RedisStep>(pSender->shared_from_this());
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool WorkerImpl::AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    size_t iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    size_t iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iFd, CODEC_NEBULA);
    if (nullptr != pChannel)
    {
        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->m_pImpl->SetIdentify(strIdentify);
        pChannel->m_pImpl->SetRemoteAddr(strHost);
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardSocketChannel(pChannel);
        }

        pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
        pChannel->m_pImpl->m_unRemoteWorkerIdx = iWorkerIndex;
        AddNamedSocketChannel(strIdentify, pChannel);
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool WorkerImpl::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s, %d, %s", strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    int iFd;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        struct hostent *he;
        he = gethostbyname(strHost.c_str());
        if (he != nullptr)
        {
            stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
        }
        else
        {
            LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
            return(false);
        }
    }
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    std::shared_ptr<SocketChannel> pChannel = nullptr;
    std::string strSchema = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(':'));
    std::transform(strSchema.begin(), strSchema.end(), strSchema.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
    if (strSchema == std::string("https"))
    {
        pChannel = CreateSocketChannel(iFd, CODEC_HTTP, false, true);
    }
    else
    {
        pChannel = CreateSocketChannel(iFd, CODEC_HTTP, false, false);
    }
    if (nullptr != pChannel)
    {
        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        char szIdentify[32] = {0};
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
        pChannel->m_pImpl->SetIdentify(szIdentify);
        pChannel->m_pImpl->SetRemoteAddr(strHost);
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(oHttpMsg, uiHttpStepSeq);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardSocketChannel(pChannel);
        }

        pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT, uiHttpStepSeq);
        // AddNamedSocketChannel(szIdentify, pChannel);   the channel should not add to named connection pool, because there is an uncompleted http request.
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool WorkerImpl::AutoRedisCmd(const std::string& strHost, int iPort, std::shared_ptr<RedisStep> pRedisStep)
{
    LOG4_TRACE("redisAsyncConnect(%s, %d)", strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->data = this;
    std::shared_ptr<RedisChannel> pRedisChannel = nullptr;
    try
    {
        pRedisChannel = std::make_shared<RedisChannel>();
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new RedisChannel error: %s", e.what());
        return(false);
    }
    pRedisChannel->listPipelineStep.push_back(pRedisStep);
    pRedisStep->SetWorker(m_pWorker);
    m_mapRedisChannel.insert(std::make_pair(c, pRedisChannel));
    redisLibevAttach(m_loop, c);
    redisAsyncSetConnectCallback(c, RedisConnectCallback);
    redisAsyncSetDisconnectCallback(c, RedisDisconnectCallback);

    std::ostringstream oss;
    oss << strHost << ":" << iPort;
    std::string strIdentify = std::move(oss.str());
    AddNamedRedisChannel(strIdentify, pRedisChannel);
    return(true);
}

bool WorkerImpl::Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    return(DiscardSocketChannel(pChannel, bChannelNotice));
}

bool WorkerImpl::Disconnect(const std::string& strIdentify, bool bChannelNotice)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter != m_mapNamedSocketChannel.end())
    {
        std::list<std::shared_ptr<SocketChannel>>::iterator channel_iter;
        while (named_iter->second.size() > 1)
        {
            channel_iter = named_iter->second.begin();
            DiscardSocketChannel(*channel_iter, bChannelNotice);
        }
        channel_iter = named_iter->second.begin();
        return(DiscardSocketChannel(*channel_iter, bChannelNotice));
    }
    return(false);
}

bool WorkerImpl::DiscardNamedChannel(const std::string& strIdentify)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_DEBUG("no channel match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        for (auto channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            (*channel_iter)->m_pImpl->SetIdentify("");
            (*channel_iter)->m_pImpl->SetClientData("");
        }
        named_iter->second.clear();
        m_mapNamedSocketChannel.erase(named_iter);
        return(true);
    }
}

bool WorkerImpl::SwitchCodec(std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eCodecType)
{
    return(pChannel->m_pImpl->SwitchCodec(eCodecType, m_stWorkerInfo.dIoTimeout));
}

void WorkerImpl::BootLoadCmd(CJsonObject& oCmdConf)
{
    LOG4_TRACE(" ");
    int iCmd = 0;
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

void WorkerImpl::DynamicLoadCmd(CJsonObject& oDynamicLoadingConf)
{
    LOG4_TRACE(" ");
    int iVersion = 0;
    bool bIsload = false;
    int32 iCmd = 0;
    std::string strUrlPath;
    std::string strSoPath;
    std::unordered_map<std::string, tagSo*>::iterator so_iter;
    tagSo* pSo = nullptr;
    for (int i = 0; i < oDynamicLoadingConf.GetArraySize(); ++i)
    {
        oDynamicLoadingConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oDynamicLoadingConf[i].Get("so_path", strSoPath) && oDynamicLoadingConf[i].Get("version", iVersion))
            {
                so_iter = m_mapLoadedSo.find(strSoPath);
                if (so_iter == m_mapLoadedSo.end())
                {
                    strSoPath = m_stWorkerInfo.strWorkPath + std::string("/") + oDynamicLoadingConf[i]("so_path");
                    pSo = LoadSo(strSoPath, iVersion);
                    if (pSo != nullptr)
                    {
                        LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                        m_mapLoadedSo.insert(std::make_pair(strSoPath, pSo));
                        for (int j = 0; j < oDynamicLoadingConf[i]["cmd"].GetArraySize(); ++j)
                        {
                            oDynamicLoadingConf[i]["cmd"][j].Get("cmd", iCmd);
                            MakeSharedCmd(nullptr, oDynamicLoadingConf[i]["cmd"][j]("class"), iCmd);
                        }
                        for (int k = 0; k < oDynamicLoadingConf[i]["module"].GetArraySize(); ++k)
                        {
                            oDynamicLoadingConf[i]["module"][k].Get("path", strUrlPath);
                            MakeSharedModule(nullptr, oDynamicLoadingConf[i]["module"][k]("class"), strUrlPath);
                        }
                    }
                }
                /*
                else
                {
                    if (iVersion != so_iter->second->iVersion)
                    {
                        strSoPath = m_stWorkerInfo.strWorkPath + std::string("/") + oDynamicLoadingConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARNING("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        pSo = LoadSo(strSoPath, iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pSo != nullptr)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            delete so_iter->second;
                            so_iter->second = pSo;
                        }
                    }
                }
                */
            }
        }
        /*
        else        // 卸载动态库
        {
            if (oDynamicLoadingConf[i].Get("cmd", iCmd))
            {
                strSoPath = m_stWorkerInfo.strWorkPath + std::string("/") + oDynamicLoadingConf[i]("so_path");
                UnloadSoAndDeleteCmd(iCmd);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
        */
    }
}

WorkerImpl::tagSo* WorkerImpl::LoadSo(const std::string& strSoPath, int iVersion)
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
    pSo->iVersion = iVersion;
    return(pSo);
}

bool WorkerImpl::AddPeriodicTaskEvent()
{
    LOG4_TRACE(" ");
    ev_timer* timeout_watcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (timeout_watcher == nullptr)
    {
        LOG4_ERROR("malloc timeout_watcher error!");
        return(false);
    }
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool WorkerImpl::AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d], seq[%u]", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_io* io_watcher = pChannel->m_pImpl->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->m_pImpl->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->m_pImpl->GetFd(), EV_READ);
        ev_io_start (m_loop, io_watcher);
        return(true);
    }
    else
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool WorkerImpl::AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_io* io_watcher = pChannel->m_pImpl->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->m_pImpl->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->m_pImpl->GetFd(), EV_WRITE);
        ev_io_start (m_loop, io_watcher);
        return(true);
    }
    else
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool WorkerImpl::RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_io* io_watcher = pChannel->m_pImpl->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        return(false);
    }
    if (EV_WRITE & io_watcher->events)
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & (~EV_WRITE));
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

void WorkerImpl::SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData)
{
    pChannel->m_pImpl->SetClientData(strClientData);
}

bool WorkerImpl::AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_timer* timer_watcher = pChannel->m_pImpl->MutableTimerWatcher();
    if (nullptr == timer_watcher)
    {
        timer_watcher = pChannel->m_pImpl->AddTimerWatcher();
        if (nullptr == timer_watcher)
        {
            return(false);
        }
        // @deprecated pChannel->m_pImpl->SetKeepAlive(m_dIoTimeout);
        ev_timer_init (timer_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        return(true);
    }
    else
    {
        LOG4_TRACE("pChannel = 0x%d,  timer_watcher = 0x%d", pChannel, timer_watcher);
        ev_timer_stop(m_loop, timer_watcher);
        ev_timer_set(timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, timer_watcher);
        return(true);
    }
}

std::shared_ptr<Session> WorkerImpl::GetSession(uint32 uiSessionId)
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
        id_iter->second->SetActiveTime(ev_now(m_loop));
        return(id_iter->second);
    }
}

std::shared_ptr<Session> WorkerImpl::GetSession(const std::string& strSessionId)
{
    auto id_iter = m_mapCallbackSession.find(strSessionId);
    if (id_iter == m_mapCallbackSession.end())
    {
        return(nullptr);
    }
    else
    {
        id_iter->second->SetActiveTime(ev_now(m_loop));
        return(id_iter->second);
    }
}

std::shared_ptr<SocketChannel> WorkerImpl::CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsServer, bool bWithSsl)
{
    LOG4_DEBUG("iFd %d, codec_type %d", iFd, eCodecType);

    std::shared_ptr<SocketChannel> pChannel = nullptr;
    try
    {
        pChannel = std::make_shared<SocketChannel>(m_pLogger, iFd, GetSequence());
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new channel for fd %d error: %s", e.what());
        return(nullptr);
    }
    pChannel->m_pImpl->SetLabor(m_pWorker);
    bool bInitResult = pChannel->m_pImpl->Init(eCodecType, bIsServer);
    if (bInitResult)
    {
        m_mapSocketChannel.insert(std::make_pair(iFd, pChannel));
        return(pChannel);
    }
    else
    {
        return(nullptr);
    }
}

bool WorkerImpl::DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    LOG4_DEBUG("%s disconnect, fd %d, identify %s", pChannel->m_pImpl->GetRemoteAddr().c_str(), pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetIdentify().c_str());
    if (bChannelNotice)
    {
        ChannelNotice(pChannel, pChannel->m_pImpl->GetIdentify(), pChannel->m_pImpl->GetClientData());
    }
    bool bCloseResult = pChannel->m_pImpl->Close();
    if (!bCloseResult)
    {
        return(bCloseResult);
    }
    ev_io_stop (m_loop, pChannel->m_pImpl->MutableIoWatcher());
    if (nullptr != pChannel->m_pImpl->MutableTimerWatcher())
    {
        ev_timer_stop (m_loop, pChannel->m_pImpl->MutableTimerWatcher());
    }

    auto named_iter = m_mapNamedSocketChannel.find(pChannel->m_pImpl->GetIdentify());
    if (named_iter != m_mapNamedSocketChannel.end())
    {
        for (auto it = named_iter->second.begin();
                it != named_iter->second.end(); ++it)
        {
            if ((*it)->m_pImpl->GetSequence() == pChannel->m_pImpl->GetSequence())
            {
                named_iter->second.erase(it);
                break;
            }
        }
        if (0 == named_iter->second.size())
        {
            m_mapNamedSocketChannel.erase(named_iter);
        }
    }

    auto channel_iter = m_mapSocketChannel.find(pChannel->m_pImpl->GetFd());
    if (channel_iter != m_mapSocketChannel.end())
    {
        m_mapSocketChannel.erase(channel_iter);
    }
    return(true);
}

void WorkerImpl::Remove(std::shared_ptr<Step> pStep)
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
    if (pStep->MutableTimerWatcher() != nullptr)
    {
        ev_timer_stop (m_loop, pStep->MutableTimerWatcher());
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("erase step(seq %u)", pStep->GetSequence());
        m_mapCallbackStep.erase(callback_iter);
    }
}

void WorkerImpl::Remove(std::shared_ptr<Session> pSession)
{
    if (nullptr == pSession)
    {
        return;
    }
    if (pSession->MutableTimerWatcher() != nullptr)
    {
        ev_timer_stop (m_loop, pSession->MutableTimerWatcher());
    }
    auto iter = m_mapCallbackSession.find(pSession->GetSessionId());
    if (iter != m_mapCallbackSession.end())
    {
        LOG4_TRACE("erase session(session_id %s)", pSession->GetSessionId().c_str());
        m_mapCallbackSession.erase(iter);
    }
}

void WorkerImpl::ChannelNotice(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const std::string& strClientData)
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
        oMsgHead.set_seq(GetSequence());
        oMsgHead.set_len(oMsgBody.ByteSize());
        std::ostringstream oss;
        oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
        cmd_iter->second->m_strTraceId = std::move(oss.str());
        cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
    }
}

bool WorkerImpl::Handle(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_DEBUG("cmd %u, seq %lu", oMsgHead.cmd(), oMsgHead.seq());
    if (gc_uiCmdReq & oMsgHead.cmd())    // 新请求
    {
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        auto cmd_iter = m_mapCmd.find(gc_uiCmdBit & oMsgHead.cmd());
        if (cmd_iter != m_mapCmd.end() && cmd_iter->second != nullptr)
        {
            if (oMsgBody.trace_id().length() > 10)
            {
                cmd_iter->second->m_strTraceId = oMsgBody.trace_id();
            }
            else
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                cmd_iter->second->m_strTraceId = std::move(oss.str());
            }
            cmd_iter->second->AnyMessage(pChannel, oMsgHead, oMsgBody);
        }
        else    // 没有对应的cmd，是需由接入层转发的请求
        {
            if (CMD_REQ_SET_LOG_LEVEL == oMsgHead.cmd())
            {
                LogLevel oLogLevel;
                oLogLevel.ParseFromString(oMsgBody.data());
                LOG4_INFO("log level set to %d", oLogLevel.log_level());
                m_pLogger->SetLogLevel(oLogLevel.log_level());
            }
            else if (CMD_REQ_RELOAD_SO == oMsgHead.cmd())
            {
                CJsonObject oSoConfJson;         // TODO So config
                DynamicLoadCmd(oSoConfJson);
            }
            else
            {
                if (CODEC_NEBULA == pChannel->m_pImpl->GetCodecType())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                {
                    cmd_iter = m_mapCmd.find(CMD_REQ_TO_CLIENT);
                    if (cmd_iter != m_mapCmd.end())
                    {
                        if (oMsgBody.trace_id().length() > 10)
                        {
                            cmd_iter->second->m_strTraceId = oMsgBody.trace_id();
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                            cmd_iter->second->m_strTraceId = std::move(oss.str());
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
                            cmd_iter->second->m_strTraceId = oMsgBody.trace_id();
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                            cmd_iter->second->m_strTraceId = std::move(oss.str());
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
            SendTo(pChannel, oOutMsgHead.cmd(), oOutMsgHead.seq(), oOutMsgBody);
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
                step_iter->second->SetActiveTime(ev_now(m_loop));
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x, active_time %lf",
                                oMsgHead.cmd(), oMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second, step_iter->second->GetActiveTime());
                eResult = (std::dynamic_pointer_cast<PbStep>(step_iter->second))->Callback(pChannel, oMsgHead, oMsgBody);
                if (CMD_STATUS_RUNNING != eResult)
                {
                    Remove(step_iter->second);
                }
            }
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

bool WorkerImpl::Handle(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
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
                HttpMsg oOutHttpMsg;
                snprintf(m_pErrBuff, gc_iErrBuffLen, "no module to dispose %s!", oHttpMsg.path().c_str());
                LOG4_ERROR(m_pErrBuff);
                oOutHttpMsg.set_type(HTTP_RESPONSE);
                oOutHttpMsg.set_status_code(404);
                oOutHttpMsg.set_http_major(oHttpMsg.http_major());
                oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
                pChannel->m_pImpl->Send(oOutHttpMsg, 0);
            }
            else
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                module_iter->second->m_strTraceId = std::move(oss.str());
                module_iter->second->AnyMessage(pChannel, oHttpMsg);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
            module_iter->second->m_strTraceId = std::move(oss.str());
            module_iter->second->AnyMessage(pChannel, oHttpMsg);
        }
    }
    else
    {
        auto http_step_iter = m_mapCallbackStep.find(pChannel->m_pImpl->GetStepSeq());
        if (http_step_iter == m_mapCallbackStep.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oHttpMsg.url().c_str());
            pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
            AddNamedSocketChannel(pChannel->m_pImpl->GetIdentify(), pChannel);       // push back to named socket channel pool.
        }
        else
        {
            E_CMD_STATUS eResult;
            http_step_iter->second->SetActiveTime(ev_now(m_loop));
            eResult = (std::dynamic_pointer_cast<HttpStep>(http_step_iter->second))->Callback(pChannel, oHttpMsg);
            if (CMD_STATUS_RUNNING != eResult)
            {
                Remove(http_step_iter->second);
                pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
                AddNamedSocketChannel(pChannel->m_pImpl->GetIdentify(), pChannel);       // push back to named socket channel pool.
            }
        }
    }
    return(true);
}

} /* namespace neb */
