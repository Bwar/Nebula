/*******************************************************************************
 * Project:  Nebula
 * @file     Dispatcher.cpp
 * @brief    事件管理、事件分发
 * @author   Bwar
 * @date:    2019年9月7日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Dispatcher.hpp"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <algorithm>
#include "util/process_helper.h"
#include "Definition.hpp"
#include "labor/Labor.hpp"
#include "labor/Manager.hpp"
#include "labor/Worker.hpp"
#include "actor/Actor.hpp"
#include "actor/step/Step.hpp"
#include "actor/step/RedisStep.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"
#include "Nodes.hpp"

namespace neb
{

Dispatcher::Dispatcher(Labor* pLabor, std::shared_ptr<NetLogger> pLogger)
   : m_pErrBuff(NULL), m_pLabor(pLabor), m_loop(NULL), m_iClientNum(0),
     m_pLogger(pLogger), m_pSessionNode(nullptr)
{
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);

    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
/*#if __cplusplus >= 201401L
    m_pSessionNode = std::make_unique<Nodes>();
#else
    m_pSessionNode = std::unique_ptr<Nodes>(new Nodes());
#endif*/
}

Dispatcher::~Dispatcher()
{
    Destroy();
}

void Dispatcher::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = static_cast<SocketChannel*>(watcher->data);
        Dispatcher* pDispatcher = pChannel->m_pImpl->m_pLabor->GetDispatcher();
        std::shared_ptr<SocketChannel> pSharedChannel = pChannel->shared_from_this();
        if (revents & EV_READ)
        {
            pDispatcher->OnIoRead(pSharedChannel);
        }
        if ((revents & EV_WRITE) && (CHANNEL_STATUS_CLOSED != pChannel->m_pImpl->GetChannelStatus())) // the channel maybe closed by OnIoRead()
        {
            pDispatcher->OnIoWrite(pSharedChannel);
        }
        if (revents & EV_ERROR)
        {
            pDispatcher->OnIoError(pSharedChannel);
        }
    }
}

void Dispatcher::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = static_cast<SocketChannel*>(watcher->data);
        Dispatcher* pDispatcher = pChannel->m_pImpl->m_pLabor->GetDispatcher();
        if (pChannel->m_pImpl->GetFd() < 3)      // TODO 这个判断是不得已的做法，需查找fd为0回调到这里的原因
        {
            return;
        }
        pDispatcher->OnIoTimeout(pChannel->shared_from_this());
    }
}

void Dispatcher::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Labor* pLabor = (Labor*)c->data;
        pLabor->GetDispatcher()->OnRedisConnected(c, status);
    }
}

void Dispatcher::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Labor* pLabor = (Labor*)c->data;
        pLabor->GetDispatcher()->OnRedisDisconnected(c, status);
    }
}

void Dispatcher::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        Labor* pLabor = (Labor*)c->data;
        pLabor->GetDispatcher()->OnRedisCmdResult(c, reply, privdata);
    }
}

void Dispatcher::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Dispatcher* pDispatcher = (Dispatcher*)(watcher->data);
        if (Labor::LABOR_MANAGER == pDispatcher->m_pLabor->GetLaborType())
        {
            ((Manager*)(pDispatcher->m_pLabor))->GetSessionManager()->CheckWorker();
            ((Manager*)(pDispatcher->m_pLabor))->RefreshServer();
        }
        else
        {
            ((Worker*)(pDispatcher->m_pLabor))->CheckParent();
        }
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Dispatcher::SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Labor* pLabor = (Labor*)watcher->data;
        if (SIGCHLD == watcher->signum)
        {
            ((Manager*)pLabor)->OnChildTerminated(watcher);
        }
        else
        {
            pLabor->OnTerminated(watcher);
        }
    }
}

void Dispatcher::ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagClientConnWatcherData* pData = (tagClientConnWatcherData*)watcher->data;
        pData->pDispatcher->OnClientConnFrequencyTimeout(pData, watcher);
    }
}

bool Dispatcher::OnIoRead(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d]", pChannel->m_pImpl->GetFd());
    if (Labor::LABOR_MANAGER == m_pLabor->GetLaborType())
    {
        if (pChannel->m_pImpl->GetFd() == ((Manager*)m_pLabor)->GetManagerInfo().iS2SListenFd)
        {
            return(AcceptServerConn(pChannel->m_pImpl->GetFd()));
        }
        else if (((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd > 2
                && pChannel->m_pImpl->GetFd() == ((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd)
        {
            return(AcceptFdAndTransfer(((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd,
                    ((Manager*)m_pLabor)->GetManagerInfo().iC2SFamily));
        }
        else
        {
            return(DataRecvAndHandle(pChannel));
        }
    }
    else
    {
        if (pChannel->m_pImpl->GetFd() == ((Worker*)m_pLabor)->GetWorkerInfo().iDataFd)
        {
            return(FdTransfer(pChannel->m_pImpl->GetFd()));
        }
        else
        {
            return(DataRecvAndHandle(pChannel));
        }
    }
}

bool Dispatcher::DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel)
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
                m_pLabor->GetActorBuilder()->OnMessage(pChannel, oHttpMsg);
            }
            else if (CODEC_STATUS_WANT_WRITE == eCodecStatus)
            {
                return(SendTo(pChannel));
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
            if (m_pLabor->GetNodeInfo().bIsAccess && !pChannel->m_pImpl->IsChannelVerify())
            {
                if (CODEC_NEBULA != pChannel->m_pImpl->GetCodecType() && pChannel->m_pImpl->GetMsgNum() > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                {
                    LOG4_DEBUG("invalid request, please login first!");
                    DiscardSocketChannel(pChannel);
                    return(false);
                }
            }
            m_pLabor->GetActorBuilder()->OnMessage(pChannel, oMsgHead, oMsgBody);
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->m_pImpl->Fetch(oMsgHead, oMsgBody);
        }
    }

    if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        return(true);
    }
    else if (CODEC_STATUS_WANT_WRITE == eCodecStatus)
    {
        return(SendTo(pChannel));
    }
    else if (CODEC_STATUS_WANT_READ == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    else    // 编解码出错或连接关闭或连接中断
    {
        LOG4_TRACE("codec error or connection closed!");
        DiscardSocketChannel(pChannel);
        return(false);
    }
}

bool Dispatcher::FdTransfer(int iFd)
{
    LOG4_TRACE(" ");
    int iAcceptFd = -1;
    int iAiFamily = AF_INET;
    int iCodec = 0;
    int iErrno = SocketChannel::RecvChannelFd(iFd, iAcceptFd, iAiFamily, iCodec, m_pLogger);
    if (iErrno != ERR_OK)
    {
        if (iErrno == ERR_CHANNEL_EOF)
        {
            LOG4_WARNING("recv_fd from fd %d error %d", iFd, errno);
            Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        if (iAiFamily != PF_UNIX)
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
        }
        std::shared_ptr<SocketChannel> pChannel = nullptr;
        LOG4_TRACE("fd[%d] transfer successfully.", iAcceptFd);
        if (CODEC_NEBULA != iCodec && ((Worker*)m_pLabor)->WithSsl())
        {
            pChannel = CreateSocketChannel(iAcceptFd, E_CODEC_TYPE(iCodec), false, true);
        }
        else
        {
            pChannel = CreateSocketChannel(iAcceptFd, E_CODEC_TYPE(iCodec), false, false);
        }
        if (nullptr != pChannel)
        {
            if (AF_INET == iAiFamily)
            {
                char szClientAddr[64] = {0};
                int z;                          /* status return code */
                struct sockaddr_in stClientAddr;
                socklen_t iClientAddrSize = sizeof(stClientAddr);
                z = getpeername(iAcceptFd, (struct sockaddr *)&stClientAddr, &iClientAddrSize);
                if (z == 0)
                {
                    inet_ntop(AF_INET, &stClientAddr.sin_addr, szClientAddr, sizeof(szClientAddr));
                    LOG4_TRACE("set fd %d's remote addr \"%s\"", iAcceptFd, szClientAddr);
                    pChannel->m_pImpl->SetRemoteAddr(std::string(szClientAddr));
                }
                else
                {
                    LOG4_ERROR("getpeername error %d", errno);
                }
            }
            else if (AF_INET6 == iAiFamily)  // AF_INET6
            {
                char szClientAddr[64] = {0};
                int z;                          /* status return code */
                struct sockaddr_in6 stClientAddr;
                socklen_t iClientAddrSize = sizeof(stClientAddr);
                z = getpeername(iAcceptFd, (struct sockaddr *)&stClientAddr, &iClientAddrSize);
                if (z == 0)
                {
                    inet_ntop(AF_INET6, &stClientAddr.sin6_addr, szClientAddr, sizeof(szClientAddr));
                    LOG4_TRACE("set fd %d's remote addr \"%s\"", iAcceptFd, szClientAddr);
                    pChannel->m_pImpl->SetRemoteAddr(std::string(szClientAddr));
                }
                else
                {
                    LOG4_ERROR("getpeername error %d", errno);
                }
            }
            AddIoReadEvent(pChannel);
            if (CODEC_NEBULA == iCodec)
            {
                AddIoTimeout(pChannel, m_pLabor->GetNodeInfo().dIoTimeout);
                std::shared_ptr<Step> pStepTellWorker
                    = m_pLabor->GetActorBuilder()->MakeSharedStep(nullptr, "neb::StepTellWorker", pChannel);
                if (nullptr == pStepTellWorker)
                {
                    return(false);
                }
                pStepTellWorker->Emit(ERR_OK);
            }
            else if (CODEC_NEBULA_IN_NODE == iCodec)
            {
                pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
                m_mapLoaderAndWorkerChannel.insert(std::make_pair(pChannel->GetFd(), pChannel));
                m_iterLoaderAndWorkerChannel = m_mapLoaderAndWorkerChannel.begin();
            }
            else
            {
                pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
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

bool Dispatcher::OnIoWrite(std::shared_ptr<SocketChannel> pChannel)
{
    if (CODEC_NEBULA == pChannel->m_pImpl->GetCodecType())  // 系统内部Server间通信
    {
        if (CHANNEL_STATUS_TRY_CONNECT == pChannel->m_pImpl->GetChannelStatus())  // connect之后的第一个写事件
        {
            if (Labor::LABOR_MANAGER == m_pLabor->GetLaborType())
            {
                MsgBody oMsgBody;
                oMsgBody.set_data(std::to_string(pChannel->m_pImpl->m_unRemoteWorkerIdx));
                LOG4_DEBUG("send after connect, oMsgBody.ByteSize() = %d", oMsgBody.ByteSize());
                SendTo(pChannel, CMD_REQ_CONNECT_TO_WORKER, m_pLabor->GetSequence(), oMsgBody);
                return(true);
            }
            else
            {
                std::shared_ptr<Step> pStepConnectWorker = m_pLabor->GetActorBuilder()->MakeSharedStep(
                        nullptr, "neb::StepConnectWorker", pChannel, pChannel->m_pImpl->m_unRemoteWorkerIdx);
                if (nullptr == pStepConnectWorker)
                {
                    LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
                    return(false);
                }
                if (CMD_STATUS_RUNNING != pStepConnectWorker->Emit(ERR_OK))
                {
                    m_pLabor->GetActorBuilder()->RemoveStep(pStepConnectWorker);
                }
                return(true);
            }
        }
    }
    else
    {
        if (CHANNEL_STATUS_CLOSED != pChannel->m_pImpl->GetChannelStatus())
        {
            pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
        }
    }

    E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send();
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_PAUSE == eCodecStatus || CODEC_STATUS_WANT_WRITE == eCodecStatus)
    {
        AddIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_WANT_READ == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else
    {
        DiscardSocketChannel(pChannel);
    }
    return(true);
}

bool Dispatcher::OnIoError(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    return(false);
}

bool Dispatcher::OnIoTimeout(std::shared_ptr<SocketChannel> pChannel)
{
    ev_tstamp after = pChannel->m_pImpl->GetActiveTime() - ev_now(m_loop) + m_pLabor->GetNodeInfo().dIoTimeout;
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
        std::shared_ptr<Step> pStepIoTimeout = m_pLabor->GetActorBuilder()->MakeSharedStep(
                nullptr, "neb::StepIoTimeout", pChannel);
        if (nullptr == pStepIoTimeout)
        {
            LOG4_ERROR("new StepIoTimeout error!");
            DiscardSocketChannel(pChannel);
        }
        E_CMD_STATUS eStatus = pStepIoTimeout->Emit();
        if (CMD_STATUS_RUNNING != eStatus)
        {
            // 若返回非running状态，则表明发包时已出错，
            // 销毁连接过程在SendTo里已经完成，这里不需要再销毁连接
            m_pLabor->GetActorBuilder()->RemoveStep(pStepIoTimeout);
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

bool Dispatcher::OnRedisConnected(const redisAsyncContext *c, int status)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        if (!m_pLabor->GetActorBuilder()->OnRedisConnected(channel_iter->second, c, status))
        {
            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            m_mapRedisChannel.erase(channel_iter);
        }
    }
    return(true);
}

bool Dispatcher::OnRedisDisconnected(const redisAsyncContext *c, int status)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        m_pLabor->GetActorBuilder()->OnRedisDisconnected(channel_iter->second, c, status);
        DelNamedRedisChannel(channel_iter->second->GetIdentify());
        m_mapRedisChannel.erase(channel_iter);
    }
    //redisAsyncDisconnect(const_cast<redisAsyncContext*>(c));  //被动断开连接不需要
    return(true);
}

bool Dispatcher::OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_TRACE(" ");
    auto channel_iter = m_mapRedisChannel.find((redisAsyncContext*)c);
    if (channel_iter != m_mapRedisChannel.end())
    {
        m_pLabor->GetActorBuilder()->OnRedisCmdResult(channel_iter->second, c, reply, privdata);
        if (nullptr == reply)
        {
            DelNamedRedisChannel(channel_iter->second->GetIdentify());
            m_mapRedisChannel.erase(channel_iter);
        }
    }
    return(true);
}

bool Dispatcher::OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher)
{
    bool bRes = false;
    auto iter = m_mapClientConnFrequency.find(std::string(pData->pAddr));
    if (iter == m_mapClientConnFrequency.end())
    {
        bRes = false;
    }
    else
    {
        m_mapClientConnFrequency.erase(iter);
        bRes = true;
    }

    DelEvent(watcher);
    pData->pDispatcher = nullptr;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

void Dispatcher::EeventRun()
{
    ev_run (m_loop, 0);
}

bool Dispatcher::AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_timer* timer_watcher = pChannel->m_pImpl->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(timer_watcher))
        {
            ev_timer_stop(m_loop, timer_watcher);
            ev_timer_set(timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, timer_watcher);
        }
        else
        {
            ev_timer_init (timer_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        return(true);
    }
}

bool Dispatcher::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("channel %d", pChannel->m_pImpl->GetFd());
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send();
    if (CODEC_STATUS_OK == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_PAUSE == eStatus || CODEC_STATUS_WANT_WRITE == eStatus)
    {
        AddIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_WANT_READ == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    return(false);
}

bool Dispatcher::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    if (nullptr != pSender)
    {
        (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
    }
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
    if (CODEC_STATUS_OK == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_PAUSE == eStatus || CODEC_STATUS_WANT_WRITE == eStatus)
    {
        AddIoWriteEvent(pChannel);
        return(true);
    }
    else if (CODEC_STATUS_WANT_READ == eStatus)
    {
        RemoveIoWriteEvent(pChannel);
        return(true);
    }
    return(false);
}

bool Dispatcher::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
        }
        return(AutoSend(strIdentify, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
        }
        E_CODEC_STATUS eStatus = (*named_iter->second.begin())->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus)
        {
            RemoveIoWriteEvent(*named_iter->second.begin());
            return(true);
        }
        else if (CODEC_STATUS_PAUSE == eStatus) // || CODEC_STATUS_WANT_WRITE == eCodecStatus)
        {
            AddIoWriteEvent(*named_iter->second.begin());
            return(true);
        }
        else if (CODEC_STATUS_WANT_READ == eStatus)
        {
            RemoveIoWriteEvent(*named_iter->second.begin());
            return(true);
        }
        return(false);
    }
}

bool Dispatcher::SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
        }
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool Dispatcher::SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("nody_type: %s, factor: %d", strNodeType.c_str(), uiFactor);
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
        }
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool Dispatcher::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
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
                    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
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
            return(SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody, pSender));
        }
    }
    else
    {
        LOG4_ERROR("MsgBody is not a request message!");
        return(false);
    }
};

bool Dispatcher::Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::unordered_set<std::string> setOnlineNodes;
    if (m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
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
        if ("BEACON" == strNodeType)
        {
            LOG4_WARNING("no beacon config.");
        }
        else
        {
            LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        }
        return(false);
    }
}

bool Dispatcher::SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    if (m_pLabor->GetLaborType() == Labor::LABOR_MANAGER)
    {
        return(Broadcast("BEACON", iCmd, uiSeq, oMsgBody, pSender));
    }
    else
    {
        auto pChannel = ((Worker*)m_pLabor)->GetManagerControlChannel();
        if (pChannel == nullptr)
        {
            LOG4_ERROR("no connected channel to manager!");
            return(false);
        }
        if (nullptr != pSender)
        {
            (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
        }
        E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus)
        {
            RemoveIoWriteEvent(pChannel);
            return(true);
        }
        else if (CODEC_STATUS_PAUSE == eStatus || CODEC_STATUS_WANT_WRITE == eStatus)
        {
            AddIoWriteEvent(pChannel);
            return(true);
        }
        else if (CODEC_STATUS_WANT_READ == eStatus)
        {
            RemoveIoWriteEvent(pChannel);
            return(true);
        }
        return(false);
    }
}

bool Dispatcher::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(oHttpMsg, uiHttpStepSeq);
    switch (eStatus)
    {
        case CODEC_STATUS_OK:
            return(true);
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_WANT_WRITE:
            AddIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_WANT_READ:
            RemoveIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_EOF:      // http1.0 respone and close
            DiscardSocketChannel(pChannel);
            return(true);
        default:
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

bool Dispatcher::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
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
            named_iter->second.erase(channel_iter);     // erase from named channel pool, the channel remain in m_mapSocketChannel.
            if (CODEC_STATUS_OK == eStatus)
            {
                return(true);
            }
            else if (CODEC_STATUS_PAUSE == eStatus || CODEC_STATUS_WANT_WRITE == eStatus)
            {
                AddIoWriteEvent(*channel_iter);
                return(true);
            }
            else if (CODEC_STATUS_WANT_READ == eStatus)
            {
                RemoveIoWriteEvent(*channel_iter);
                return(true);
            }
            return(false);
        }
    }
}

bool Dispatcher::SendTo(std::shared_ptr<RedisChannel> pRedisChannel, Actor* pSender)
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

bool Dispatcher::SendTo(const std::string& strIdentify, Actor* pSender)
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

bool Dispatcher::SendTo(const std::string& strHost, int iPort, Actor* pSender)
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

bool Dispatcher::AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    size_t iPosIpPortSeparator = strIdentify.rfind(':');
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

    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult;
    struct addrinfo* pAddrCurrent;
    memset(&stAddrHints, 0, sizeof(struct addrinfo));
    stAddrHints.ai_family = AF_UNSPEC;
    stAddrHints.ai_socktype = SOCK_STREAM;
    stAddrHints.ai_protocol = IPPROTO_IP;
    int iCode = getaddrinfo(strHost.c_str(), strPort.c_str(), &stAddrHints, &pAddrResult);
    if (0 != iCode)
    {
        LOG4_ERROR("getaddrinfo(\"%s\", \"%s\") error %d: %s",
                strHost.c_str(), strPort.c_str(), iCode, gai_strerror(iCode));
        return(false);
    }
    int iFd = -1;
    for (pAddrCurrent = pAddrResult;
            pAddrCurrent != NULL; pAddrCurrent = pAddrCurrent->ai_next)
    {
        iFd = socket(pAddrCurrent->ai_family,
                pAddrCurrent->ai_socktype, pAddrCurrent->ai_protocol);
        if (iFd == -1)
        {
            continue;
        }

        break;
    }

    /* No address succeeded */
    if (pAddrCurrent == NULL)
    {
        LOG4_ERROR("Could not connect to \"%s:%s\"", strHost.c_str(), strPort.c_str());
        freeaddrinfo(pAddrResult);           /* No longer needed */
        return(false);
    }

    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iFd, CODEC_NEBULA);
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        freeaddrinfo(pAddrResult);           /* No longer needed */
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->m_pImpl->SetIdentify(strIdentify);
        pChannel->m_pImpl->SetRemoteAddr(strHost);
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK != eCodecStatus
                && CODEC_STATUS_PAUSE != eCodecStatus
                && CODEC_STATUS_WANT_WRITE != eCodecStatus
                && CODEC_STATUS_WANT_READ != eCodecStatus)
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
        freeaddrinfo(pAddrResult);           /* No longer needed */
        close(iFd);
        return(false);
    }
}

bool Dispatcher::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s, %d, %s", strHost.c_str(), iPort, strUrlPath.c_str());
    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult;
    struct addrinfo* pAddrCurrent;
    memset(&stAddrHints, 0, sizeof(struct addrinfo));
    stAddrHints.ai_family = AF_UNSPEC;
    stAddrHints.ai_socktype = SOCK_STREAM;
    stAddrHints.ai_protocol = IPPROTO_IP;
    int iCode = getaddrinfo(strHost.c_str(), std::to_string(iPort).c_str(), &stAddrHints, &pAddrResult);
    if (0 != iCode)
    {
        LOG4_ERROR("getaddrinfo(\"%s\", \"%d\") error %d: %s",
                strHost.c_str(), iPort, iCode, gai_strerror(iCode));
        return(false);
    }
    int iFd = -1;
    for (pAddrCurrent = pAddrResult;
            pAddrCurrent != NULL; pAddrCurrent = pAddrCurrent->ai_next)
    {
        iFd = socket(pAddrCurrent->ai_family,
                pAddrCurrent->ai_socktype, pAddrCurrent->ai_protocol);
        if (iFd == -1)
        {
            continue;
        }

        break;
    }

    /* No address succeeded */
    if (pAddrCurrent == NULL)
    {
        LOG4_ERROR("Could not connect to \"%s:%d\"", strHost.c_str(), iPort);
        freeaddrinfo(pAddrResult);           /* No longer needed */
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
        pChannel = CreateSocketChannel(iFd, CODEC_HTTP, true, true);
    }
    else
    {
        pChannel = CreateSocketChannel(iFd, CODEC_HTTP, true, false);
    }
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        freeaddrinfo(pAddrResult);           /* No longer needed */
        char szIdentify[32] = {0};
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
        pChannel->m_pImpl->SetIdentify(szIdentify);
        pChannel->m_pImpl->SetRemoteAddr(strHost);
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(oHttpMsg, uiHttpStepSeq);
        if (CODEC_STATUS_OK != eCodecStatus
                && CODEC_STATUS_PAUSE != eCodecStatus
                && CODEC_STATUS_WANT_WRITE != eCodecStatus
                && CODEC_STATUS_WANT_READ != eCodecStatus)
        {
            DiscardSocketChannel(pChannel);
        }

        pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT, uiHttpStepSeq);
        // AddNamedSocketChannel(szIdentify, pChannel);   the channel should not add to named connection pool, because there is an uncompleted http request.
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        freeaddrinfo(pAddrResult);           /* No longer needed */
        close(iFd);
        return(false);
    }
}

bool Dispatcher::AutoRedisCmd(const std::string& strHost, int iPort, std::shared_ptr<RedisStep> pRedisStep)
{
    LOG4_TRACE("redisAsyncConnect(%s, %d)", strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        LOG4_ERROR("error: %s", c->errstr);
        // If the onConnect callback is called with REDIS_ERROR the context will
        // be disconnected (and the inner context freed) after that callback anyway.
        // No need to call it yourself
        // redisAsyncFree(c);
        return(false);
    }
    c->data = m_pLabor;
    std::shared_ptr<RedisChannel> pRedisChannel = nullptr;
    try
    {
        pRedisChannel = std::make_shared<RedisChannel>(c);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new RedisChannel error: %s", e.what());
        return(false);
    }
    pRedisChannel->listPipelineStep.push_back(pRedisStep);
    pRedisStep->SetLabor(m_pLabor);
    m_mapRedisChannel.insert(std::make_pair(c, pRedisChannel));
    redisLibevAttach(m_loop, c);
    redisAsyncSetConnectCallback(c, RedisConnectCallback);
    redisAsyncSetDisconnectCallback(c, RedisDisconnectCallback);

    std::ostringstream oss;
    oss << strHost << ":" << iPort;
    std::string strIdentify = std::move(oss.str());
    pRedisChannel->SetIdentify(strIdentify);
    AddNamedRedisChannel(strIdentify, pRedisChannel);
    return(true);
}

bool Dispatcher::SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    if (m_pLabor->GetLaborType() == Labor::LABOR_MANAGER)
    {
        LOG4_ERROR("this function can not be called by manager process!");
        return(false);
    }
    if (nullptr != pSender)
    {
        (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pSender->GetTraceId());
    }
    if (m_iterLoaderAndWorkerChannel == m_mapLoaderAndWorkerChannel.end())
    {
        m_iterLoaderAndWorkerChannel = m_mapLoaderAndWorkerChannel.begin();
        if (m_iterLoaderAndWorkerChannel == m_mapLoaderAndWorkerChannel.end())
        {
            LOG4_ERROR("no channel for loader!");
            return(false);
        }
    }
    E_CODEC_STATUS eStatus = m_iterLoaderAndWorkerChannel->second->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
    if (CODEC_STATUS_OK == eStatus)
    {
        RemoveIoWriteEvent(m_iterLoaderAndWorkerChannel->second);
        m_iterLoaderAndWorkerChannel++;
        return(true);
    }
    else if (CODEC_STATUS_PAUSE == eStatus || CODEC_STATUS_WANT_WRITE == eStatus)
    {
        AddIoWriteEvent(m_iterLoaderAndWorkerChannel->second);
        m_iterLoaderAndWorkerChannel++;
        return(true);
    }
    else if (CODEC_STATUS_WANT_READ == eStatus)
    {
        RemoveIoWriteEvent(m_iterLoaderAndWorkerChannel->second);
        m_iterLoaderAndWorkerChannel++;
        return(true);
    }
    m_iterLoaderAndWorkerChannel++;
    return(false);
}

bool Dispatcher::SendTo(int iFd, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    auto iter = m_mapSocketChannel.find(iFd);
    if (iter != m_mapSocketChannel.end())
    {
        return(SendTo(iter->second, iCmd, uiSeq, oMsgBody));
    }
    return(false);
}

bool Dispatcher::Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    return(DiscardSocketChannel(pChannel, bChannelNotice));
}

bool Dispatcher::Disconnect(const std::string& strIdentify, bool bChannelNotice)
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

bool Dispatcher::DiscardNamedChannel(const std::string& strIdentify)
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

bool Dispatcher::SwitchCodec(std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eCodecType)
{
    return(pChannel->m_pImpl->SwitchCodec(eCodecType, m_pLabor->GetNodeInfo().dIoTimeout));
}

bool Dispatcher::AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel)
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

void Dispatcher::DelNamedSocketChannel(const std::string& strIdentify)
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

void Dispatcher::SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify)
{
    pChannel->m_pImpl->SetIdentify(strIdentify);
}

bool Dispatcher::AddNamedRedisChannel(const std::string& strIdentify, std::shared_ptr<RedisChannel> pChannel)
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

void Dispatcher::DelNamedRedisChannel(const std::string& strIdentify)
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

void Dispatcher::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->AddNode(strNodeType, strIdentify);

    if (std::string("BEACON") != m_pLabor->GetNodeInfo().strNodeType
            && std::string("LOGGER") != m_pLabor->GetNodeInfo().strNodeType)
    {
        std::string strOnlineNode;
        if (std::string("LOGGER") == strNodeType && m_pSessionNode->GetNode(strNodeType, strOnlineNode))
        {
            m_pLogger->EnableNetLogger(true);
        }
    }
}

void Dispatcher::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->DelNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType
            && !m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(false);
    }
}

void Dispatcher::SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData)
{
    pChannel->m_pImpl->SetClientData(strClientData);
}

bool Dispatcher::IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType)
{
    return(m_pSessionNode->IsNodeType(strNodeIdentify, strNodeType));
}

bool Dispatcher::AddEvent(ev_signal* signal_watcher, signal_callback pFunc, int iSignum)
{
    if (NULL == signal_watcher)
    {
        return(false);
    }
    ev_signal_init (signal_watcher, pFunc, iSignum);
    ev_signal_start (m_loop, signal_watcher);
    return(true);
}

bool Dispatcher::AddEvent(ev_timer* timer_watcher, timer_callback pFunc, ev_tstamp dTimeout)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_init (timer_watcher, pFunc, dTimeout + ev_time() - ev_now(m_loop), 0.);
    ev_timer_start (m_loop, timer_watcher);
    return(true);
}

bool Dispatcher::RefreshEvent(ev_timer* timer_watcher, ev_tstamp dTimeout)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_stop (m_loop, timer_watcher);
    ev_timer_set (timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, timer_watcher);
    return(true);
}

bool Dispatcher::DelEvent(ev_io* io_watcher)
{
    if (NULL == io_watcher)
    {
        return(false);
    }
    ev_io_stop (m_loop, io_watcher);
    return(true);
}

bool Dispatcher::DelEvent(ev_timer* timer_watcher)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_stop (m_loop, timer_watcher);
    return(true);
}

int Dispatcher::SendFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType)
{
    return(SocketChannel::SendChannelFd(iSocketFd, iSendFd, iAiFamily, iCodecType, m_pLogger));
}

bool Dispatcher::CreateListenFd(const std::string& strHost, int32 iPort, int& iFd, int& iFamily)
{
    int queueLen = 100;
    int reuse = 1;
    int timeout = 1;

    struct addrinfo stAddrHints;
    struct addrinfo* pAddrResult;
    struct addrinfo* pAddrCurrent;
    memset(&stAddrHints, 0, sizeof(struct addrinfo));
    stAddrHints.ai_family = AF_UNSPEC;              /* Allow IPv4 or IPv6 */
    stAddrHints.ai_socktype = SOCK_STREAM;
    stAddrHints.ai_protocol = IPPROTO_IP;
    stAddrHints.ai_flags = AI_PASSIVE;              /* For wildcard IP address */
    int iCode = getaddrinfo(strHost.c_str(),
            std::to_string(iPort).c_str(), &stAddrHints, &pAddrResult);
    if (0 != iCode)
    {
        LOG4_ERROR("getaddrinfo(\"%s\", \"%d\") error %d: %s",
                strHost.c_str(),
                iPort, iCode, gai_strerror(iCode));
        exit(errno);
    }
    for (pAddrCurrent = pAddrResult;
            pAddrCurrent != NULL; pAddrCurrent = pAddrCurrent->ai_next)
    {
        iFd = socket(pAddrCurrent->ai_family,
                pAddrCurrent->ai_socktype, pAddrCurrent->ai_protocol);
        if (-1 == iFd)
        {
            continue;
        }
        if (-1 == ::setsockopt(iFd,
                    SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == ::setsockopt(iFd,
                    IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int)))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == bind(iFd,
                    pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen))
        {
            close(iFd);
            iFd = -1;
            continue;
        }
        if (-1 == listen(iFd, queueLen))
        {
            close(iFd);
            iFd = -1;
            continue;
        }

        iFamily = pAddrCurrent->ai_family;
        break;
    }
    freeaddrinfo(pAddrResult);           /* No longer needed */

    /* No address succeeded */
    if (-1 == iFd)
    {
        LOG4_ERROR("Could not bind to \"%s:%d\", error %d: %s",
                strHost.c_str(),
                iPort,
                errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        return(false);
    }
    return(true);
}

std::shared_ptr<SocketChannel> Dispatcher::GetChannel(int iFd)
{
    auto iter = m_mapSocketChannel.find(iFd);
    if (iter != m_mapSocketChannel.end())
    {
        return(iter->second);
    }
    return(nullptr);
}

int32 Dispatcher::GetConnectionNum() const
{
    return((int32)m_mapSocketChannel.size());
}
int32 Dispatcher::GetClientNum() const
{
    return(m_iClientNum);
}

bool Dispatcher::Init()
{
#if __cplusplus >= 201401L
    m_pSessionNode = std::make_unique<Nodes>();
#else
    m_pSessionNode = std::unique_ptr<Nodes>(new Nodes());
#endif
    return(true);
}

void Dispatcher::Destroy()
{
    m_mapSocketChannel.clear();
    m_mapRedisChannel.clear();
    m_mapNamedSocketChannel.clear();
    m_mapNamedRedisChannel.clear();
    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
    if (m_pErrBuff != NULL)
    {
        free(m_pErrBuff);
        m_pErrBuff = NULL;
    }
    m_pLabor = nullptr;
}

std::shared_ptr<SocketChannel> Dispatcher::CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsClient, bool bWithSsl)
{
    LOG4_DEBUG("iFd %d, codec_type %d, with_ssl = %d", iFd, eCodecType, bWithSsl);

    auto iter = m_mapSocketChannel.find(iFd);
    if (iter == m_mapSocketChannel.end())
    {
        std::shared_ptr<SocketChannel> pChannel = nullptr;
        try
        {
            pChannel = std::make_shared<SocketChannel>(m_pLogger, iFd, m_pLabor->GetSequence(), bWithSsl);
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new channel for fd %d error: %s", e.what());
            return(nullptr);
        }
        pChannel->m_pImpl->SetLabor(m_pLabor);
        bool bInitResult = pChannel->m_pImpl->Init(eCodecType, bIsClient);
        if (bInitResult)
        {
            m_mapSocketChannel.insert(std::make_pair(iFd, pChannel));
            if (CODEC_NEBULA != eCodecType)
            {
                ++m_iClientNum;
            }
            return(pChannel);
        }
        else
        {
            return(nullptr);
        }
    }
    else
    {
        LOG4_WARNING("fd %d is exist!", iFd);
        return(iter->second);
    }
}

bool Dispatcher::DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice)
{
    if (pChannel == nullptr)
    {
        LOG4_DEBUG("pChannel not exist!");
        return(false);
    }
    LOG4_DEBUG("%s disconnect, fd %d, channel_seq %u, identify %s",
            pChannel->m_pImpl->GetRemoteAddr().c_str(),
            pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence(),
            pChannel->m_pImpl->GetIdentify().c_str());
    if (bChannelNotice)
    {
        m_pLabor->GetActorBuilder()->ChannelNotice(pChannel, pChannel->m_pImpl->GetIdentify(), pChannel->m_pImpl->GetClientData());
    }
    bool bCloseResult = pChannel->m_pImpl->Close();
    if (bCloseResult)
    {
        ev_io_stop (m_loop, pChannel->m_pImpl->MutableIoWatcher());
        if (nullptr != pChannel->m_pImpl->MutableTimerWatcher())
        {
            ev_timer_stop (m_loop, pChannel->m_pImpl->MutableTimerWatcher());
        }

        if (CODEC_NEBULA_IN_NODE == pChannel->m_pImpl->GetCodecType())
        {
            auto inner_channel_iter = m_mapLoaderAndWorkerChannel.find(pChannel->GetFd());
            m_mapLoaderAndWorkerChannel.erase(inner_channel_iter);
            m_iterLoaderAndWorkerChannel = m_mapLoaderAndWorkerChannel.begin();
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
                    LOG4_TRACE("erase channel %d from m_mapNamedSocketChannel.", pChannel->m_pImpl->GetFd());
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
            if (CODEC_NEBULA != pChannel->m_pImpl->GetCodecType())
            {
                --m_iClientNum;
            }
            LOG4_TRACE("erase channel %d channel_seq %u from m_mapSocketChannel.",
                    pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
        }
        return(true);
    }
    else
    {
        return(bCloseResult);
    }
}

bool Dispatcher::AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d], seq[%u]", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_io* io_watcher = pChannel->m_pImpl->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(io_watcher))
        {
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            ev_io_init (io_watcher, IoCallback, pChannel->m_pImpl->GetFd(), EV_READ);
            ev_io_start (m_loop, io_watcher);
        }
        return(true);
    }
}

bool Dispatcher::AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_io* io_watcher = pChannel->m_pImpl->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        return(false);
    }
    else
    {
        if (ev_is_active(io_watcher))
        {
            ev_io_stop(m_loop, io_watcher);
            ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
            ev_io_start (m_loop, io_watcher);
        }
        else
        {
            ev_io_init (io_watcher, IoCallback, pChannel->m_pImpl->GetFd(), EV_WRITE);
            ev_io_start (m_loop, io_watcher);
        }
        return(true);
    }
}
bool Dispatcher::RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
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

void Dispatcher::SetChannelStatus(std::shared_ptr<SocketChannel> pChannel, E_CHANNEL_STATUS eStatus)
{
    pChannel->m_pImpl->SetChannelStatus(eStatus);
}

void Dispatcher::SetChannelStatus(std::shared_ptr<SocketChannel> pChannel, E_CHANNEL_STATUS eStatus, uint32 ulStepSeq)
{
    pChannel->m_pImpl->SetChannelStatus(eStatus, ulStepSeq);
}

bool Dispatcher::AddClientConnFrequencyTimeout(const char* pAddr, ev_tstamp dTimeout)
{
    LOG4_TRACE(" ");
    ev_timer* timeout_watcher = new ev_timer();
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    tagClientConnWatcherData* pData = new tagClientConnWatcherData();
    if (pData == NULL)
    {
        LOG4_ERROR("new tagClientConnWatcherData error!");
        delete timeout_watcher;
        return(false);
    }
    ev_timer_init (timeout_watcher, ClientConnFrequencyTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
    pData->pDispatcher = this;
    snprintf(pData->pAddr, gc_iAddrLen, "%s", pAddr);
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Dispatcher::AcceptFdAndTransfer(int iFd, int iFamily)
{
    char szClientAddr[64] = {0};
    int iAcceptFd = -1;
    if (AF_INET == iFamily)
    {
        struct sockaddr_in stClientAddr;
        socklen_t clientAddrSize = sizeof(stClientAddr);
        iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
        if (iAcceptFd < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
            return(false);
        }
        inet_ntop(AF_INET, &stClientAddr.sin_addr, szClientAddr, sizeof(szClientAddr));
        LOG4_TRACE("accept connect from \"%s\"", szClientAddr);
    }
    else    // AF_INET6
    {
        struct sockaddr_in6 stClientAddr;
        socklen_t clientAddrSize = sizeof(stClientAddr);
        iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
        if (iAcceptFd < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
            return(false);
        }
        inet_ntop(AF_INET6, &stClientAddr.sin6_addr, szClientAddr, sizeof(szClientAddr));
        LOG4_TRACE("accept connect from \"%s\"", szClientAddr);
    }

    auto iter = m_mapClientConnFrequency.find(std::string(szClientAddr));
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::make_pair(std::string(szClientAddr), 1));
        AddClientConnFrequencyTimeout(szClientAddr, m_pLabor->GetNodeInfo().dAddrStatInterval);
    }
    else
    {
        iter->second++;
        if (((Manager*)m_pLabor)->GetManagerInfo().iC2SListenFd > 2 && iter->second > (uint32)m_pLabor->GetNodeInfo().iAddrPermitNum)
        {
            LOG4_WARNING("client addr %s had been connected more than %u times in %f seconds, it's not permitted",
                            szClientAddr, m_pLabor->GetNodeInfo().iAddrPermitNum, m_pLabor->GetNodeInfo().dAddrStatInterval);
            close(iAcceptFd);
            return(false);
        }
    }

    int iWorkerDataFd = -1;
    //std::pair<int, int> worker_pid_fd = ((Manager*)m_pLabor)->GetSessionManager()->GetMinLoadWorkerDataFd();
    //iWorkerDataFd = worker_pid_fd.second;
    iWorkerDataFd = ((Manager*)m_pLabor)->GetSessionManager()->GetNextWorkerDataFd();
    if (iWorkerDataFd > 0)
    {
        LOG4_DEBUG("send new fd %d to worker communication fd %d",
                        iAcceptFd, iWorkerDataFd);
        int iCodec = m_pLabor->GetNodeInfo().eCodec;
        int iErrno = SocketChannel::SendChannelFd(iWorkerDataFd, iAcceptFd, iFamily, iCodec, m_pLogger);
        if (iErrno != ERR_OK)
        {
            LOG4_ERROR("error %d: %s", iErrno, strerror_r(iErrno, m_pErrBuff, gc_iErrBuffLen));
        }
        close(iAcceptFd);
        return(true);
    }
    LOG4_WARNING("GetMinLoadWorkerDataFd() found worker_pid_fd.second = %d", iWorkerDataFd);
    close(iAcceptFd);
    return(false);
}

bool Dispatcher::AcceptServerConn(int iFd)
{
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        return(false);
    }
    else
    {
        /* tcp连接检测 */
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
            LOG4_WARNING("fail to set SO_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
        x_sock_set_block(iAcceptFd, 0);
        std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iAcceptFd, CODEC_NEBULA);
        if (NULL != pChannel)
        {
            AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            AddIoReadEvent(pChannel);
        }
    }
    return(false);
}

void Dispatcher::EvBreak()
{
    ev_break (m_loop, EVBREAK_ALL);
}

}
