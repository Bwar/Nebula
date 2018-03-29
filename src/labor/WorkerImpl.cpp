/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2018年3月27日
 * @note
 * Modify history:
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#include "util/process_helper.h"
#include "util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include <labor/WorkerImpl.hpp>
#include <actor/step/Step.hpp>
#include <actor/step/RedisStep.hpp>
#include <actor/step/HttpStep.hpp>
#include <actor/session/Session.hpp>
#include <actor/cmd/Cmd.hpp>
#include <actor/cmd/Module.hpp>
#include <actor/cmd/sys_cmd/CmdConnectWorker.hpp>
#include <actor/cmd/sys_cmd/CmdToldWorker.hpp>
#include <actor/cmd/sys_cmd/CmdUpdateNodeId.hpp>
#include <actor/cmd/sys_cmd/CmdNodeNotice.hpp>
#include <actor/cmd/sys_cmd/CmdBeat.hpp>
#include <actor/step/sys_step/StepIoTimeout.hpp>

//#include <iostream>

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
        SocketChannel* pChannel = (SocketChannel*)watcher->data;
        Worker* pWorker = (Worker*)pChannel->m_pLabor;
        if (revents & EV_READ)
        {
            pWorker->m_pImpl->IoRead(pChannel);
        }
        if (revents & EV_WRITE)
        {
            pWorker->m_pImpl->IoWrite(pChannel);
        }
        if (CHANNEL_STATUS_DISCARD == pChannel->GetChannelStatus() || CHANNEL_STATUS_DESTROY == pChannel->GetChannelStatus())
        {
            DELETE(pChannel);
        }
    }
}

void WorkerImpl::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = (SocketChannel*)watcher->data;
        Worker* pWorker = (Worker*)(pChannel->m_pLabor);
        if (pChannel->GetFd() < 3)      // TODO 这个判断是不得已的做法，需查找fd为0回调到这里的原因
        {
            return;
        }
        pWorker->m_pImpl->IoTimeout(pChannel);
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
        ((Worker*)(pStep->m_pWorker))->m_pImpl->StepTimeout(pStep);
    }
}

void WorkerImpl::SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((Worker*)pSession->m_pWorker)->m_pImpl->SessionTimeout(pSession);
    }
}

void WorkerImpl::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->RedisConnected(c, status);
    }
}

void WorkerImpl::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->RedisDisconnected(c, status);
    }
}

void WorkerImpl::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->m_pImpl->RedisCmdResult(c, reply, privdata);
    }
}

WorkerImpl::WorkerImpl(Worker* pWorker, const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf)
    : m_pErrBuff(nullptr), m_pWorker(pWorker), m_bInitLogger(false),
      m_loop(nullptr), m_pCmdConnect(nullptr)
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
    if (!CreateEvents())
    {
        exit(-2);
    }
    LoadSysCmd();
    LoadCmd(oJsonConf["dynamic_loading"]);
    Run();
}

WorkerImpl::~WorkerImpl()
{
    Destroy();
}

void WorkerImpl::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

void WorkerImpl::Terminated(struct ev_signal* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iSignum = watcher->signum;
    delete watcher;
    //Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    exit(iSignum);
}

bool WorkerImpl::CheckParent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_stWorkerInfo.iWorkerIndex);
        //Destroy();
        exit(0);
    }
    MsgBody oMsgBody;
    CJsonObject oJsonLoad;
    oJsonLoad.Add("load", int32(m_mapSocketChannel.size() + m_mapCallbackStep.size()));
    oJsonLoad.Add("connect", int32(m_mapSocketChannel.size()));
    oJsonLoad.Add("recv_num", m_stWorkerInfo.iRecvNum);
    oJsonLoad.Add("recv_byte", m_stWorkerInfo.iRecvByte);
    oJsonLoad.Add("send_num", m_stWorkerInfo.iSendNum);
    oJsonLoad.Add("send_byte", m_stWorkerInfo.iSendByte);
    oJsonLoad.Add("client", int32(m_mapSocketChannel.size() - m_mapInnerFd.size()));
    oMsgBody.set_data(oJsonLoad.ToString());
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(m_stWorkerInfo.iManagerControlFd);
    if (iter != m_mapSocketChannel.end())
    {
        iter->second->Send(CMD_REQ_UPDATE_WORKER_LOAD, GetSequence(), oMsgBody);
    }
    m_stWorkerInfo.iRecvNum = 0;
    m_stWorkerInfo.iRecvByte = 0;
    m_stWorkerInfo.iSendNum = 0;
    m_stWorkerInfo.iSendByte = 0;
    return(true);
}

bool WorkerImpl::IoRead(SocketChannel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pChannel->GetFd() == m_stWorkerInfo.iManagerDataFd)
    {
        return(FdTransfer());
    }
    else
    {
        return(RecvDataAndHandle(pChannel));
    }
}

bool WorkerImpl::RecvDataAndHandle(SocketChannel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    E_CODEC_STATUS eCodecStatus;
    if (CODEC_HTTP == pChannel->GetCodecType())
    {
        for (int i = 0; ; ++i)
        {
            HttpMsg oHttpMsg;
            if (0 == i)
            {
                eCodecStatus = pChannel->Recv(oHttpMsg);
            }
            else
            {
                eCodecStatus = pChannel->Fetch(oHttpMsg);
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
        eCodecStatus = pChannel->Recv(oMsgHead, oMsgBody);
        while (CODEC_STATUS_OK == eCodecStatus)
        {
#ifdef NODE_TYPE_ACCESS
            if (!pChannel->IsChannelVerify())
            {
                std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(pChannel->GetFd());
                if (inner_iter == m_mapInnerFd.end() && pChannel->GetMsgNum() > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                {
                    LOG4_DEBUG("invalid request, please login first!");
                    DiscardChannel(pChannel);
                    return(false);
                }
            }
#endif
            Handle(pChannel, oMsgHead, oMsgBody);
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->Fetch(oMsgHead, oMsgBody);
        }
    }

    if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        return(true);
    }
    else    // 编解码出错或连接关闭或连接中断
    {
        DiscardChannel(pChannel);
        return(false);
    }
}

bool WorkerImpl::FdTransfer()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    int iCodec = 0;
    // int iAcceptFd = recv_fd(m_iManagerDataFd);
    int iAcceptFd = recv_fd_with_attr(m_stWorkerInfo.iManagerDataFd, szIpAddr, 16, &iCodec);
    if (iAcceptFd <= 0)
    {
        if (iAcceptFd == 0)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d len %d", m_stWorkerInfo.iManagerDataFd, iAcceptFd);
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
        else if (errno != EAGAIN)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d", m_stWorkerInfo.iManagerDataFd, errno);
            //Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        SocketChannel* pChannel = CreateChannel(iAcceptFd, E_CODEC_TYPE(iCodec));
        if (NULL != pChannel)
        {
            int z;                          /* status return code */
            struct sockaddr_in adr_inet;    /* AF_INET */
            unsigned int len_inet;                   /* length */
            z = getpeername(iAcceptFd, (struct sockaddr *)&adr_inet, &len_inet);
            pChannel->SetRemoteAddr(inet_ntoa(adr_inet.sin_addr));
            AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            AddIoReadEvent(pChannel);
            if (CODEC_PROTOBUF != iCodec)
            {
                pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            }
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
            close(iAcceptFd);
        }
    }
    return(false);
}

bool WorkerImpl::IoWrite(SocketChannel* pChannel)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    if (CHANNEL_STATUS_TRY_CONNECT == pChannel->GetChannelStatus())  // connect之后的第一个写事件
    {
        auto index_iter = m_mapSeq2WorkerIndex.find(pChannel->GetSequence());
        if (index_iter != m_mapSeq2WorkerIndex.end())
        {
            tagChannelContext stCtx;
            stCtx.iFd = pChannel->GetFd();
            stCtx.uiSeq = pChannel->GetSequence();
            AddInnerChannel(stCtx);
            if (CODEC_PROTOBUF == pChannel->GetCodecType())  // 系统内部Server间通信
            {
                ((CmdConnectWorker*)m_pCmdConnect)->Start(stCtx, index_iter->second);
            }
            m_mapSeq2WorkerIndex.erase(index_iter);
            return(false);
        }
    }

    if (CODEC_PROTOBUF != pChannel->GetCodecType()
            && CHANNEL_STATUS_ESTABLISHED != pChannel->GetChannelStatus())
    {
        pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    }

    E_CODEC_STATUS eCodecStatus = pChannel->Send();
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
        DiscardChannel(pChannel);
    }
    return(true);
}

bool WorkerImpl::IoTimeout(SocketChannel* pChannel)
{
    ev_tstamp after = pChannel->GetActiveTime() - ev_now(m_loop) + m_stWorkerInfo.dIoTimeout;
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, pChannel->MutableTimerWatcher());
        ev_timer_set (pChannel->MutableTimerWatcher(), after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, pChannel->MutableTimerWatcher());
        return(true);
    }

    LOG4_TRACE("%s(fd %d, seq %u):", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    if (pChannel->NeedAliveCheck())     // 需要发送心跳检查
    {
        tagChannelContext stCtx;
        stCtx.iFd = pChannel->GetFd();
        stCtx.uiSeq = pChannel->GetSequence();
        StepIoTimeout* pStepIoTimeout = NewStep(nullptr, "neb::StepIoTimeout", stCtx);
        if (nullptr == pStepIoTimeout)
        {
            LOG4_ERROR("new StepIoTimeout error!");
            DiscardChannel(pChannel);
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
        auto inner_iter = m_mapInnerFd.find(pChannel->GetFd());
        if (inner_iter == m_mapInnerFd.end())   // 非内部服务器间的连接才会在超时中关闭
        {
            LOG4_TRACE("io timeout!");
            DiscardChannel(pChannel);
        }
    }
    return(true);
}

bool WorkerImpl::StepTimeout(Step* pStep)
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
        LOG4_TRACE("%s(seq %lu): active_time %lf, now_time %lf, lifetime %lf",
                        __FUNCTION__, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
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

bool WorkerImpl::SessionTimeout(Session* pSession)
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
        LOG4_TRACE("%s(session_name: %s,  session_id: %s)",
                        __FUNCTION__, pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
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

bool WorkerImpl::RedisConnected(const redisAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
                iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, RedisCmdCallback, NULL, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", (*step_iter)->CmdToString().c_str());
                }
                else
                {
                    auto interrupt_step_iter = step_iter;
                    for (; step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
                    {
                        (*step_iter)->Callback(c, status, NULL);
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
                    delete channel_iter->second;
                    channel_iter->second = NULL;
                    m_mapRedisChannel.erase(channel_iter);
                }
            }
        }
        else
        {
            for (auto step_iter = channel_iter->second->listPipelineStep.begin();
                            step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
            {
                (*step_iter)->Callback(c, status, NULL);
                Remove(*step_iter);
            }
            channel_iter->second->listPipelineStep.clear();
            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            delete channel_iter->second;
            channel_iter->second = nullptr;
            m_mapRedisChannel.erase(channel_iter);
        }
    }
    return(true);
}

bool WorkerImpl::RedisDisconnected(const redisAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        for (auto step_iter = channel_iter->second->listPipelineStep.begin();
                        step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->CmdToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            Remove(*step_iter);
        }
        channel_iter->second->listPipelineStep.clear();

        DelNamedRedisChannel(channel_iter->second->GetIdentify());
        delete channel_iter->second;
        channel_iter->second = nullptr;
        m_mapRedisChannel.erase(channel_iter);
    }
    return(true);
}

bool WorkerImpl::RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        auto step_iter = channel_iter->second->listPipelineStep.begin();
        if (NULL == reply)
        {
            LOG4_ERROR("redis %s error %d: %s", channel_iter->second->GetIdentify().c_str(), c->err, c->errstr);
            for ( ; step_iter != channel_iter->second->listPipelineStep.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->CmdToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
                delete (*step_iter);
                (*step_iter) = NULL;
            }
            channel_iter->second->listPipelineStep.clear();

            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            delete channel_iter->second;
            channel_iter->second = nullptr;
            m_mapRedisChannel.erase(channel_iter);
        }
        else
        {
            if (step_iter != channel_iter->second->listPipelineStep.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->CmdToString().c_str());
                /** @note 注意，若Callback返回STATUS_CMD_RUNNING，框架不回收并且不再管理该RedisStep，该RedisStep后续必须重新RegisterCallback或由开发者自己回收 */
                if (CMD_STATUS_RUNNING != (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply))
                {
                    Remove(*step_iter);
                    (*step_iter) = nullptr;
                }
                channel_iter->second->listPipelineStep.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    return(true);
}

time_t WorkerImpl::GetNowTime() const
{
    return((time_t)ev_now(m_loop));
}

bool WorkerImpl::ResetTimeout(Actor* pObject)
{
    ev_timer* watcher = pObject->MutableTimerWatcher();
    ev_tstamp after = ev_now(m_loop) + pObject->GetTimeout();
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
#ifdef NODE_TYPE_ACCESS
    oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_dMsgStatInterval);
    oJsonConf["permission"]["uin_permit"].Get("permit_num", m_iMsgPermitNum);
#endif
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    m_pCmdConnect = new CmdConnectWorker();
    if (m_pCmdConnect == NULL)
    {
        return(false);
    }
    m_pCmdConnect->SetLogger(&m_oLogger);
    m_pCmdConnect->SetWorker(m_pWorker);
    return(true);
}

bool WorkerImpl::InitLogger(const CJsonObject& oJsonConf)
{
    if (m_bInitLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        m_oLogger.setLogLevel(iLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iLogLevel = 0;
        int32 iLoggingPort = 9000;
        std::string strLoggingHost;
        std::string strLogname = m_stWorkerInfo.strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_stWorkerInfo.strWorkerIdentify;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        if (oJsonConf.Get("log_level", iLogLevel))
        {
            switch (iLogLevel)
            {
                case log4cplus::DEBUG_LOG_LEVEL:
                    break;
                case log4cplus::INFO_LOG_LEVEL:
                    break;
                case log4cplus::TRACE_LOG_LEVEL:
                    break;
                case log4cplus::WARN_LOG_LEVEL:
                    break;
                case log4cplus::ERROR_LOG_LEVEL:
                    break;
                case log4cplus::FATAL_LOG_LEVEL:
                    break;
                default:
                    iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
        log4cplus::initialize();
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
        append->setLayout(layout);
        m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
        m_oLogger.setLogLevel(iLogLevel);
        m_oLogger.addAppender(append);
        if (oJsonConf.Get("socket_logging_host", strLoggingHost) && oJsonConf.Get("socket_logging_port", iLoggingPort))
        {
            log4cplus::SharedAppenderPtr socket_append(new log4cplus::SocketAppender(
                            strLoggingHost, iLoggingPort, ssServerName.str()));
            socket_append->setName(ssServerName.str());
            socket_append->setLayout(layout);
            socket_append->setThreshold(log4cplus::INFO_LOG_LEVEL);
            m_oLogger.addAppender(socket_append);
        }
        LOG4_INFO("%s program begin...", getproctitle());
        m_bInitLogger = true;
        return(true);
    }
}

bool WorkerImpl::CreateEvents()
{
    m_loop = ev_loop_new(EVFLAG_AUTO);
    if (m_loop == NULL)
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
    SocketChannel* pChannelData = CreateChannel(m_stWorkerInfo.iManagerDataFd, CODEC_PROTOBUF);
    SocketChannel* pChannelControl = CreateChannel(m_stWorkerInfo.iManagerControlFd, CODEC_PROTOBUF);
    if (NULL == pChannelData || NULL == pChannelControl)
    {
        return(false);
    }
    pChannelData->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    pChannelControl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    AddIoReadEvent(pChannelData);
    // AddIoTimeout(pChannelData, 60.0);   // TODO 需要优化
    AddIoReadEvent(pChannelControl);
    // AddIoTimeout(pChannelControl, 60.0); //TODO 需要优化
    m_mapInnerFd.insert(std::make_pair(m_stWorkerInfo.iManagerDataFd, pChannelData->GetSequence()));
    m_mapInnerFd.insert(std::make_pair(m_stWorkerInfo.iManagerControlFd, pChannelControl->GetSequence()));
    return(true);
}

void WorkerImpl::LoadSysCmd()
{
    NewCmd(nullptr, "neb::CmdToldWorker", CMD_REQ_TELL_WORKER);
    NewCmd(nullptr, "neb::CmdUpdateNodeId", CMD_REQ_REFRESH_NODE_ID);
    NewCmd(nullptr, "neb::CmdNodeNotice", CMD_REQ_NODE_REG_NOTICE);
    NewCmd(nullptr, "neb::CmdBeat", CMD_REQ_BEAT);
}

void WorkerImpl::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);

    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        DELETE(cmd_iter->second);
    }
    m_mapCmd.clear();

    for (auto step_iter = m_mapCallbackStep.begin();
            step_iter != m_mapCallbackStep.end(); ++step_iter)
    {
        Remove(step_iter->second);
    }
    m_mapCallbackStep.clear();

    for (auto attr_iter = m_mapSocketChannel.begin();
                    attr_iter != m_mapSocketChannel.end(); ++attr_iter)
    {
        LOG4_TRACE("for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapChannel.begin();");
        DiscardChannel(attr_iter->second);
    }
    m_mapSocketChannel.clear();

    for (auto so_iter = m_mapLoadedSo.begin();
                    so_iter != m_mapLoadedSo.end(); ++so_iter)
    {
        DELETE(so_iter->second);
    }
    m_mapLoadedSo.clear();

    // TODO 待补充完整

    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }

    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

void WorkerImpl::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool WorkerImpl::AddNamedSocketChannel(const std::string& strIdentify, const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(%s, fd %d, seq %u)", __FUNCTION__, strIdentify.c_str(), stCtx.iFd, stCtx.uiSeq);
    auto channel_iter = m_mapSocketChannel.find(stCtx.iFd);
    if (channel_iter == m_mapSocketChannel.end())
    {
        return(false);
    }
    else
    {
        if (stCtx.uiSeq == channel_iter->second->GetSequence())
        {
            auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
            if (named_iter == m_mapNamedSocketChannel.end())
            {
                std::list<SocketChannel*> listChannel;
                listChannel.push_back(channel_iter->second);
                m_mapNamedSocketChannel.insert(std::pair<std::string, std::list<SocketChannel*> >(strIdentify, listChannel));
            }
            else
            {
                named_iter->second.push_back(channel_iter->second);
            }
            channel_iter->second->SetIdentify(strIdentify);
            return(true);
        }
        return(false);
    }
}

bool WorkerImpl::AddNamedSocketChannel(const std::string& strIdentify, SocketChannel* pChannel)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        std::list<SocketChannel*> listChannel;
        listChannel.push_back(pChannel);
        m_mapNamedSocketChannel.insert(std::pair<std::string, std::list<SocketChannel*> >(strIdentify, listChannel));
        return(true);
    }
    else
    {
        named_iter->second.push_back(pChannel);
    }
    pChannel->SetIdentify(strIdentify);
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

bool WorkerImpl::SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    auto iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter == m_mapSocketChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            iter->second->SetIdentify(strIdentify);
            return(true);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.uiSeq, iter->second->GetSequence());
            return(false);
        }
    }
}

bool WorkerImpl::AddNamedRedisChannel(const std::string& strIdentify, redisAsyncContext* pCtx)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    auto channel_iter = m_mapRedisChannel.find(pCtx);
    if (channel_iter == m_mapRedisChannel.end())
    {
        return(false);
    }
    else
    {
        auto named_iter = m_mapNamedRedisChannel.find(strIdentify);
        if (named_iter == m_mapNamedRedisChannel.end())
        {
            std::list<RedisChannel*> listChannel;
            listChannel.push_back(channel_iter->second);
            m_mapNamedRedisChannel.insert(std::make_pair(strIdentify, listChannel));
        }
        else
        {
            named_iter->second.push_back(channel_iter->second);
        }
        channel_iter->second->SetIdentify(strIdentify);
        return(true);
    }
}

bool WorkerImpl::AddNamedRedisChannel(const std::string& strIdentify, RedisChannel* pChannel)
{
    auto named_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (named_iter == m_mapNamedRedisChannel.end())
    {
        std::list<RedisChannel*> listChannel;
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
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    auto iter = m_mapIdentifyNodeType.find(strIdentify);
    if (iter == m_mapIdentifyNodeType.end())
    {
        m_mapIdentifyNodeType.insert(iter, std::make_pair(strIdentify, strNodeType));

        T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(strNodeType);
        if (node_type_iter == m_mapNodeIdentify.end())
        {
            std::set<std::string> setIdentify;
            setIdentify.insert(strIdentify);
            std::pair<T_MAP_NODE_TYPE_IDENTIFY::iterator, bool> insert_node_result;
            insert_node_result = m_mapNodeIdentify.insert(std::pair< std::string,
                            std::pair<std::set<std::string>::iterator, std::set<std::string> > >(
                                            strNodeType, std::make_pair(setIdentify.begin(), setIdentify)));    //这里的setIdentify是临时变量，setIdentify.begin()将会成非法地址
            if (insert_node_result.second == false)
            {
                return;
            }
            insert_node_result.first->second.first = insert_node_result.first->second.second.begin();
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter == node_type_iter->second.second.end())
            {
                node_type_iter->second.second.insert(strIdentify);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
    }
}

void WorkerImpl::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    auto identify_iter = m_mapIdentifyNodeType.find(strIdentify);
    if (identify_iter != m_mapIdentifyNodeType.end())
    {
        auto node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
        if (node_type_iter != m_mapNodeIdentify.end())
        {
            auto id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.second.erase(id_iter);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
        m_mapIdentifyNodeType.erase(identify_iter);
    }
}

void WorkerImpl::AddInnerChannel(const tagChannelContext& stCtx)
{
    std::map<int, uint32>::iterator iter = m_mapInnerFd.find(stCtx.iFd);
    if (iter == m_mapInnerFd.end())
    {
        m_mapInnerFd.insert(std::pair<int, uint32>(stCtx.iFd, stCtx.uiSeq));
    }
    else
    {
        iter->second = stCtx.uiSeq;
    }
    LOG4_TRACE("%s() now m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
}

bool WorkerImpl::SetClientData(const tagChannelContext& stCtx, const std::string& strClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter == m_mapSocketChannel.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            iter->second->SetClientData(strClientData);
            return(true);
        }
        else
        {
            return(false);
        }
    }
}

bool WorkerImpl::SendTo(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stCtx.iFd, stCtx.uiSeq);
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter == m_mapSocketChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            E_CODEC_STATUS eStatus = iter->second->Send();
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
    }
    return(false);
}

bool WorkerImpl::SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("%s(fd %d, fd_seq %lu, cmd %u, msg_seq %u)",
                    __FUNCTION__, stCtx.iFd, stCtx.uiSeq, uiCmd, uiSeq);
    std::map<int, SocketChannel*>::iterator conn_iter = m_mapSocketChannel.find(stCtx.iFd);
    if (conn_iter == m_mapSocketChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->GetSequence() == stCtx.uiSeq)
        {
            if (nullptr != pSender)
            {
                (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
            }
            E_CODEC_STATUS eStatus = conn_iter->second->Send(uiCmd, uiSeq, oMsgBody);
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.uiSeq, conn_iter->second->GetSequence());
            return(false);
        }
    }
}

bool WorkerImpl::SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, std::list<SocketChannel*> >::iterator named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        return(AutoSend(strIdentify, uiCmd, uiSeq, oMsgBody));
    }
    else
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
        }
        E_CODEC_STATUS eStatus = (*named_iter->second.begin())->Send(uiCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
        {
            return(true);
        }
        return(false);
    }
}

bool WorkerImpl::SendPolling(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.first != node_type_iter->second.second.end())
        {
            if (nullptr != pSender)
            {
                (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
            }
            std::set<std::string>::iterator id_iter = node_type_iter->second.first;
            node_type_iter->second.first++;
            return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
            if (id_iter != node_type_iter->second.second.end())
            {
                if (nullptr != pSender)
                {
                    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
                }
                node_type_iter->second.first = id_iter;
                node_type_iter->second.first++;
                return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
            }
            else
            {
                LOG4_ERROR("no channel match and no node identify found for %s!", strNodeType.c_str());
                return(false);
            }
        }
    }
}

bool WorkerImpl::SendOrient(const std::string& strNodeType, unsigned int uiFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("%s(nody_type: %s, factor: %d)", __FUNCTION__, strNodeType.c_str(), uiFactor);
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.second.size() == 0)
        {
            LOG4_ERROR("no channel match %s!", strNodeType.c_str());
            return(false);
        }
        else
        {
            std::set<std::string>::iterator id_iter;
            int target_identify = uiFactor % node_type_iter->second.second.size();
            int i = 0;
            for (i = 0, id_iter = node_type_iter->second.second.begin();
                            i < node_type_iter->second.second.size();
                            ++i, ++id_iter)
            {
                if (i == target_identify && id_iter != node_type_iter->second.second.end())
                {
                    if (nullptr != pSender)
                    {
                        (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
                    }
                    return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
                }
            }
            return(false);
        }
    }
}

bool WorkerImpl::Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        int iSendNum = 0;
        for (std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
                        id_iter != node_type_iter->second.second.end(); ++id_iter)
        {
            if (*id_iter != m_stWorkerInfo.strWorkerIdentify)
            {
                if (nullptr != pSender)
                {
                    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->m_strTraceId);
                }
                SendTo(*id_iter, uiCmd, uiSeq, oMsgBody);
            }
            ++iSendNum;
        }
        if (0 == iSendNum)
        {
            LOG4_ERROR("no channel match and no node identify found for %s!", strNodeType.c_str());
            return(false);
        }
    }
    return(true);
}

bool WorkerImpl::SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s(fd %d, seq %lu)", __FUNCTION__, stCtx.iFd, stCtx.uiSeq);
    std::map<int, SocketChannel*>::iterator conn_iter = m_mapSocketChannel.find(stCtx.iFd);
    if (conn_iter == m_mapSocketChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->GetSequence() == stCtx.uiSeq)
        {
            E_CODEC_STATUS eStatus = conn_iter->second->Send(oHttpMsg, uiHttpStepSeq);
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.uiSeq, conn_iter->second->GetSequence());
            return(false);
        }
    }
}

bool WorkerImpl::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, szIdentify);
    std::map<std::string, std::list<SocketChannel*> >::iterator named_iter = m_mapNamedSocketChannel.find(szIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", szIdentify);
        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
    }
    else
    {
        for (auto channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            if (0 == (*channel_iter)->GetStepSeq())
            {
                E_CODEC_STATUS eStatus = (*channel_iter)->Send(oHttpMsg, uiHttpStepSeq);
                if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
                {
                    return(true);
                }
                return(false);
            }
        }
        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
    }
}

bool WorkerImpl::SendTo(RedisChannel* pRedisChannel, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
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
        status = redisAsyncCommandArgv((redisAsyncContext*)pRedisChannel->RedisContext(), RedisCmdCallback, NULL, args_size, argv, arglen);
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

bool WorkerImpl::SendTo(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    std::ostringstream oss;
    oss << strHost << ":" << iPort;
    std::string strIdentify = std::move(oss.str());
    auto ctx_iter = m_mapNamedRedisChannel.find(strIdentify);
    if (ctx_iter != m_mapNamedRedisChannel.end())
    {
        return(SendTo(*(ctx_iter->second.begin()), pRedisStep));
    }
    else
    {
        LOG4_TRACE("m_pLabor->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool WorkerImpl::AutoSend(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
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
    SocketChannel* pChannel = CreateChannel(iFd, CODEC_PROTOBUF);
    if (NULL != pChannel)
    {
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->SetIdentify(strIdentify);
        pChannel->SetRemoteAddr(strHost);
        pChannel->SetChannelStatus(CHANNEL_STATUS_INIT);
        E_CODEC_STATUS eCodecStatus = pChannel->Send(uiCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardChannel(pChannel);
        }

        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        pChannel->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
        m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(pChannel->GetSequence(), iWorkerIndex));
        AddNamedSocketChannel(strIdentify, pChannel);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool WorkerImpl::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s(%s, %d, %s)", __FUNCTION__, strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    int iFd;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        struct hostent *he;
        he = gethostbyname(strHost.c_str());
        if (he != NULL)
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
    SocketChannel* pChannel = CreateChannel(iFd, CODEC_HTTP);
    if (NULL != pChannel)
    {
        char szIdentify[32] = {0};
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
        pChannel->SetIdentify(szIdentify);
        pChannel->SetRemoteAddr(strHost);
        pChannel->SetChannelStatus(CHANNEL_STATUS_INIT);
        E_CODEC_STATUS eCodecStatus = pChannel->Send(oHttpMsg, uiHttpStepSeq);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardChannel(pChannel);
        }

        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        pChannel->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT, uiHttpStepSeq);
        AddNamedSocketChannel(szIdentify, pChannel);
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool WorkerImpl::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->data = this;
    RedisChannel* pRedisChannel = NULL;
    try
    {
        pRedisChannel = new RedisChannel();
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new RedisChannel error: %s", e.what());
        return(false);
    }
    pRedisChannel->listPipelineStep.push_back(pRedisStep);
    pRedisStep->SetLogger(&m_oLogger);
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

bool WorkerImpl::Disconnect(const tagChannelContext& stCtx, bool bChannelNotice)
{
    auto iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter != m_mapSocketChannel.end())
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            return(DiscardChannel(iter->second, bChannelNotice));
        }
    }
    return(false);
}

bool WorkerImpl::Disconnect(const std::string& strIdentify, bool bChannelNotice)
{
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter != m_mapNamedSocketChannel.end())
    {
        std::list<SocketChannel*>::iterator channel_iter;
        while (named_iter->second.size() > 1)
        {
            channel_iter = named_iter->second.begin();
            DiscardChannel(*channel_iter, bChannelNotice);
        }
        channel_iter = named_iter->second.begin();
        return(DiscardChannel(*channel_iter, bChannelNotice));
    }
    return(false);
}

std::string WorkerImpl::GetClientAddr(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter == m_mapSocketChannel.end())
    {
        return("");
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            return(iter->second->GetRemoteAddr());
        }
        else
        {
            return("");
        }
    }
}

bool WorkerImpl::DiscardNamedChannel(const std::string& strIdentify)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_DEBUG("no channel match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        for (std::list<SocketChannel*>::iterator channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            (*channel_iter)->SetIdentify("");
            (*channel_iter)->SetClientData("");
        }
        named_iter->second.clear();
        m_mapNamedSocketChannel.erase(named_iter);
        return(true);
    }
}

bool WorkerImpl::SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter == m_mapSocketChannel.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.uiSeq)
        {
            return(iter->second->SwitchCodec(eCodecType, m_stWorkerInfo.dIoTimeout));
        }
        else
        {
            return(false);
        }
    }
}

void WorkerImpl::LoadCmd(CJsonObject& oDynamicLoadingConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iVersion = 0;
    bool bIsload = false;
    int32 iCmd = 0;
    std::string strUrlPath;
    std::string strSoPath;
    std::unordered_map<std::string, tagSo*>::iterator so_iter;
    tagSo* pSo = NULL;
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
                            NewCmd(nullptr, oDynamicLoadingConf[i]["cmd"][j]("class"), iCmd);
                        }
                        for (int k = 0; k < oDynamicLoadingConf[i]["module"].GetArraySize(); ++k)
                        {
                            oDynamicLoadingConf[i]["module"][k].Get("path", strUrlPath);
                            NewModule(nullptr, oDynamicLoadingConf[i]["module"][k]("class"), strUrlPath);
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
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
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
    LOG4_TRACE("%s()", __FUNCTION__);
    tagSo* pSo = nullptr;
    void* pHandle = nullptr;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        if (pHandle != nullptr)
        {
            dlclose(pHandle);
        }
        return(pSo);
    }
    pSo->pSoHandle = pHandle;
    pSo->iVersion = iVersion;
    return(pSo);
}

bool WorkerImpl::AddPeriodicTaskEvent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("malloc timeout_watcher error!");
        return(false);
    }
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool WorkerImpl::AddIoReadEvent(SocketChannel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_READ);
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

bool WorkerImpl::AddIoWriteEvent(SocketChannel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_WRITE);
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

bool WorkerImpl::RemoveIoWriteEvent(SocketChannel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
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

bool WorkerImpl::AddIoTimeout(SocketChannel* pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_timer* timer_watcher = pChannel->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        timer_watcher = pChannel->AddTimerWatcher();
        LOG4_TRACE("pChannel = 0x%d,  timer_watcher = 0x%d", pChannel, timer_watcher);
        if (NULL == timer_watcher)
        {
            return(false);
        }
        // @deprecated pChannel->SetKeepAlive(m_dIoTimeout);
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

bool WorkerImpl::AddIoTimeout(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, stCtx.iFd, stCtx.uiSeq);
    std::map<int, SocketChannel*>::iterator iter = m_mapSocketChannel.find(stCtx.iFd);
    if (iter != m_mapSocketChannel.end())
    {
        if (stCtx.uiSeq == iter->second->GetSequence())
        {
            ev_timer* timer_watcher = iter->second->MutableTimerWatcher();
            if (NULL == timer_watcher)
            {
                timer_watcher = iter->second->AddTimerWatcher();
                LOG4_TRACE("timer_watcher = 0x%d", timer_watcher);
                if (NULL == timer_watcher)
                {
                    return(false);
                }
                ev_timer_init (timer_watcher, IoTimeoutCallback, iter->second->GetKeepAlive() + ev_time() - ev_now(m_loop), 0.);
                ev_timer_start (m_loop, timer_watcher);
                return(true);
            }
            else
            {
                LOG4_TRACE("timer_watcher = 0x%d", timer_watcher);
                ev_timer_stop(m_loop, timer_watcher);
                ev_timer_set(timer_watcher, iter->second->GetKeepAlive() + ev_time() - ev_now(m_loop), 0);
                ev_timer_start (m_loop, timer_watcher);
                return(true);
            }
        }
    }
    return(false);
}

Session* WorkerImpl::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    auto name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(nullptr);
    }
    else
    {
        std::ostringstream oss;
        oss << uiSessionId;
        auto id_iter = name_iter->second.find(oss.str());
        if (id_iter == name_iter->second.end())
        {
            return(nullptr);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

Session* WorkerImpl::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    auto name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(nullptr);
    }
    else
    {
        auto id_iter = name_iter->second.find(strSessionId);
        if (id_iter == name_iter->second.end())
        {
            return(nullptr);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

SocketChannel* WorkerImpl::CreateChannel(int iFd, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(iFd %d, codec_type %d)", __FUNCTION__, iFd, eCodecType);
    std::map<int, SocketChannel*>::iterator iter;
    iter = m_mapSocketChannel.find(iFd);
    if (iter == m_mapSocketChannel.end())
    {
        SocketChannel* pChannel = NULL;
        try
        {
            pChannel = new SocketChannel(iFd, GetSequence());
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new channel for fd %d error: %s", e.what());
            return(NULL);
        }
        pChannel->SetLabor(m_pWorker);
        pChannel->SetLogger(&m_oLogger);
        if (pChannel->Init(eCodecType))
        {
            m_mapSocketChannel.insert(std::pair<int, SocketChannel*>(iFd, pChannel));
            return(pChannel);
        }
        else
        {
            DELETE(pChannel);
            return(NULL);
        }
    }
    else
    {
        LOG4_WARN("fd %d is exist!", iFd);
        return(iter->second);
    }
}

bool WorkerImpl::DiscardChannel(SocketChannel* pChannel, bool bChannelNotice)
{
    LOG4_DEBUG("%s disconnect, identify %s", pChannel->GetRemoteAddr().c_str(), pChannel->GetIdentify().c_str());
    if (CHANNEL_STATUS_DISCARD == pChannel->GetChannelStatus() || CHANNEL_STATUS_DESTROY == pChannel->GetChannelStatus())
    {
        return(false);
    }
    auto inner_iter = m_mapInnerFd.find(pChannel->GetFd());
    if (inner_iter != m_mapInnerFd.end())
    {
        LOG4_TRACE("%s() m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
        m_mapInnerFd.erase(inner_iter);
    }
    auto named_iter = m_mapNamedSocketChannel.find(pChannel->GetIdentify());
    if (named_iter != m_mapNamedSocketChannel.end())
    {
        for (std::list<SocketChannel*>::iterator it = named_iter->second.begin();
                it != named_iter->second.end(); ++it)
        {
            if ((*it)->GetSequence() == pChannel->GetFd())
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
    if (bChannelNotice)
    {
        tagChannelContext stCtx;
        stCtx.iFd = pChannel->GetFd();
        stCtx.uiSeq = pChannel->GetSequence();
        ChannelNotice(stCtx, pChannel->GetIdentify(), pChannel->GetClientData());
    }
    ev_io_stop (m_loop, pChannel->MutableIoWatcher());
    if (NULL != pChannel->MutableTimerWatcher())
    {
        ev_timer_stop (m_loop, pChannel->MutableTimerWatcher());
    }
    auto channel_iter = m_mapSocketChannel.find(pChannel->GetFd());
    if (channel_iter != m_mapSocketChannel.end() && channel_iter->second->GetSequence() == pChannel->GetSequence())
    {
        m_mapSocketChannel.erase(channel_iter);
    }
    pChannel->SetChannelStatus(CHANNEL_STATUS_DISCARD);
    return(true);
}

void WorkerImpl::Remove(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, pStep);
    if (nullptr == pStep)
    {
        return;
    }
    std::unordered_map<uint32, Step*>::iterator callback_iter;
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
            ResetTimeout(pStep);
            return;
        }
    }
    if (pStep->MutableTimerWatcher() != NULL)
    {
        ev_timer_stop (m_loop, pStep->MutableTimerWatcher());
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("delete step(seq %u)", pStep->GetSequence());
        DELETE(pStep);
        m_mapCallbackStep.erase(callback_iter);
    }
}

void WorkerImpl::Remove(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X)", __FUNCTION__, pSession);
    if (nullptr == pSession)
    {
        return;
    }
    if (pSession->MutableTimerWatcher() != NULL)
    {
        ev_timer_stop (m_loop, pSession->MutableTimerWatcher());
    }
    auto callback_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (callback_iter != m_mapCallbackSession.end())
    {
        auto iter = callback_iter->second.find(pSession->GetSessionId());
        if (iter != callback_iter->second.end())
        {
            LOG4_TRACE("delete session(session_name %s, session_id %s)",
                            pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
            DELETE(pSession);
            callback_iter->second.erase(iter);
            if (callback_iter->second.empty())
            {
                m_mapCallbackSession.erase(callback_iter);
            }
        }
    }
}

void WorkerImpl::ChannelNotice(const tagChannelContext& stCtx, const std::string& strIdentify, const std::string& strClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    auto cmd_iter = m_mapCmd.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapCmd.end() && cmd_iter->second != NULL)
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
        cmd_iter->second->AnyMessage(stCtx, oMsgHead, oMsgBody);
    }
}

bool WorkerImpl::Handle(SocketChannel* pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %lu)", __FUNCTION__, oMsgHead.cmd(), oMsgHead.seq());
    tagChannelContext stCtx;
    stCtx.iFd = pChannel->GetFd();
    stCtx.uiSeq = pChannel->GetSequence();
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
            cmd_iter->second->AnyMessage(stCtx, oMsgHead, oMsgBody);
        }
        else    // 没有对应的cmd，是需由接入层转发的请求
        {
            if (CMD_REQ_SET_LOG_LEVEL == oMsgHead.cmd())
            {
                LogLevel oLogLevel;
                oLogLevel.ParseFromString(oMsgBody.data());
                LOG4_INFO("log level set to %d", oLogLevel.log_level());
                m_oLogger.setLogLevel(oLogLevel.log_level());
            }
            else if (CMD_REQ_RELOAD_SO == oMsgHead.cmd())
            {
                CJsonObject oSoConfJson;         // TODO So config
                LoadCmd(oSoConfJson);
            }
            else
            {
//#ifdef NODE_TYPE_ACCESS
                auto inner_iter = m_mapInnerFd.find(stCtx.iFd);
                if (inner_iter != m_mapInnerFd.end())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
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
                        cmd_iter->second->AnyMessage(stCtx, oMsgHead, oMsgBody);
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
                        cmd_iter->second->AnyMessage(stCtx, oMsgHead, oMsgBody);
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
            SendTo(stCtx, oOutMsgHead.cmd(), oOutMsgHead.seq(), oOutMsgBody);
            return(false);
        }
    }
    else    // 回调
    {
        std::map<uint32, Step*>::iterator step_iter;
        step_iter = m_mapCallbackStep.find(oMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",
                            oMsgHead.cmd());
            if (step_iter->second != NULL)
            {
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(ev_now(m_loop));
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x, active_time %lf",
                                oMsgHead.cmd(), oMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second, step_iter->second->GetActiveTime());
                eResult = ((PbStep*)step_iter->second)->Callback(stCtx, oMsgHead, oMsgBody);
                if (CMD_STATUS_RUNNING != eResult)
                {
                    Remove(step_iter->second);
                }
            }
        }
        else
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!", oMsgHead.seq());
            LOG4_WARN(m_pErrBuff);
            return(false);
        }
    }
    return(true);
}

bool WorkerImpl::Handle(SocketChannel* pChannel, const HttpMsg& oHttpMsg)
{
    LOG4_DEBUG("%s() oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    __FUNCTION__, oHttpMsg.type(), oHttpMsg.path().c_str());
    tagChannelContext stCtx;
    stCtx.iFd = pChannel->GetFd();
    stCtx.uiSeq = pChannel->GetSequence();
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
                pChannel->Send(oOutHttpMsg, 0);
            }
            else
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                module_iter->second->m_strTraceId = std::move(oss.str());
                module_iter->second->AnyMessage(stCtx, oHttpMsg);
            }
        }
        else
        {
            std::ostringstream oss;
            oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
            module_iter->second->m_strTraceId = std::move(oss.str());
            module_iter->second->AnyMessage(stCtx, oHttpMsg);
        }
    }
    else
    {
        std::map<uint32, Step*>::iterator http_step_iter = m_mapCallbackStep.find(pChannel->GetStepSeq());
        if (http_step_iter == m_mapCallbackStep.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oHttpMsg.url().c_str());
            pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
        }
        else
        {
            E_CMD_STATUS eResult;
            http_step_iter->second->SetActiveTime(ev_now(m_loop));
            eResult = ((HttpStep*)http_step_iter->second)->Callback(stCtx, oHttpMsg);
            if (CMD_STATUS_RUNNING != eResult)
            {
                Remove(http_step_iter->second);
                pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
            }
        }
    }
    return(true);
}

} /* namespace neb */
