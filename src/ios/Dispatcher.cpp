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
#include <algorithm>
#include "Definition.hpp"
#include "labor/Labor.hpp"
#include "labor/Manager.hpp"
#include "labor/Worker.hpp"
#include "actor/Actor.hpp"
#include "actor/step/Step.hpp"
#include "actor/step/RedisStep.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

Dispatcher::Dispatcher(Labor* pLabor, std::shared_ptr<NetLogger> pLogger)
   : m_pErrBuff(NULL), m_pLabor(pLabor), m_loop(NULL), m_iClientNum(0),
     m_pLogger(pLogger), m_pSessionNode(nullptr)
{
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);

    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
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
        Dispatcher* pDispatcher = pChannel->m_pImpl->GetLabor()->GetDispatcher();
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
        Dispatcher* pDispatcher = pChannel->m_pImpl->GetLabor()->GetDispatcher();
        if (pChannel->m_pImpl->GetFd() < 3)      // TODO 这个判断是不得已的做法，需查找fd为0回调到这里的原因
        {
            return;
        }
        pDispatcher->OnIoTimeout(pChannel->shared_from_this());
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
        pDispatcher->CheckFailedNode();
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
    else if (Labor::LABOR_WORKER == m_pLabor->GetLaborType() || Labor::LABOR_LOADER == m_pLabor->GetLaborType())
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
    else
    {
        return(DataRecvAndHandle(pChannel));
    }
}

bool Dispatcher::DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    E_CODEC_STATUS eCodecStatus;
    switch(pChannel->GetCodecType())
    {
        case CODEC_HTTP:
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
                else if (CODEC_STATUS_EOF == eCodecStatus && oHttpMsg.ByteSize() > 10) // http1.0 client close
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oHttpMsg);
                }
                else
                {
                    break;
                }
            }
            break;
        case CODEC_RESP:
            for (int i = 0; ; ++i)
            {
                RedisMsg oRedisMsg;
                if (0 == i)
                {
                    eCodecStatus = pChannel->m_pImpl->Recv(oRedisMsg);
                }
                else
                {
                    eCodecStatus = pChannel->m_pImpl->Fetch(oRedisMsg);
                }

                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oRedisMsg);
                }
                else
                {
                    break;
                }
            }
            break;
        case CODEC_UNKNOW:
        {
            CBuffer oBuff;
            for (int i = 0; ; ++i)
            {
                RedisMsg oRedisMsg;
                if (0 == i)
                {
                    eCodecStatus = pChannel->m_pImpl->Recv(oBuff);
                }
                else
                {
                    eCodecStatus = pChannel->m_pImpl->Fetch(oBuff);
                }

                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oBuff);
                }
                else
                {
                    break;
                }
            }
        }
            break;
        default:
            for (int i = 0; ; ++i)
            {
                MsgHead oMsgHead;
                MsgBody oMsgBody;
                if (0 == i)
                {
                    eCodecStatus = pChannel->m_pImpl->Recv(oMsgHead, oMsgBody);
                }
                else
                {
                    eCodecStatus = pChannel->m_pImpl->Fetch(oMsgHead, oMsgBody);
                }

                if (CODEC_STATUS_OK == eCodecStatus)
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
                }
                else
                {
                    break;
                }
            }
    }

    switch (eCodecStatus)
    {
        case CODEC_STATUS_PAUSE: 
            return(true);
        case CODEC_STATUS_WANT_WRITE: 
            return(SendTo(pChannel));
        case CODEC_STATUS_WANT_READ: 
            RemoveIoWriteEvent(pChannel);
            return(true);
        default: // 编解码出错或连接关闭或连接中断
            if (CODEC_STATUS_INVALID == eCodecStatus && !pChannel->IsClient())
            {
                if (pChannel->m_pImpl->AutoSwitchCodec())
                {
                    return(DataFetchAndHandle(pChannel));
                }
            }
            if (CHANNEL_STATUS_ESTABLISHED != pChannel->m_pImpl->GetChannelStatus())
            {
                if (pChannel->IsClient())
                {
                    m_pSessionNode->NodeFailed(pChannel->GetIdentify());
                }
                auto& listUncompletedStep = pChannel->m_pImpl->GetPipelineStepSeq();
                for (auto it = listUncompletedStep.begin();
                        it != listUncompletedStep.end(); ++it)
                {
                    m_pLabor->GetActorBuilder()->OnError(pChannel, *it,
                            pChannel->m_pImpl->GetErrno(), pChannel->m_pImpl->GetErrMsg());
                }
            }
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

bool Dispatcher::DataFetchAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    E_CODEC_STATUS eCodecStatus;
    switch(pChannel->GetCodecType())
    {
        case CODEC_HTTP:
            {
                HttpMsg oHttpMsg;
                eCodecStatus = pChannel->m_pImpl->Fetch(oHttpMsg);
                while (CODEC_STATUS_OK == eCodecStatus)
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oHttpMsg);
                    eCodecStatus = pChannel->m_pImpl->Fetch(oHttpMsg);
                }
                if (CODEC_STATUS_EOF == eCodecStatus && oHttpMsg.ByteSize() > 10) // http1.0 client close
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oHttpMsg);
                }
            }
            break;
        case CODEC_RESP:
            for (int i = 0; ; ++i)
            {
                RedisMsg oRedisMsg;
                eCodecStatus = pChannel->m_pImpl->Fetch(oRedisMsg);
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oRedisMsg);
                }
                else
                {
                    break;
                }
            }
            break;
        case CODEC_UNKNOW:
        {
            CBuffer oBuff;
            for (int i = 0; ; ++i)
            {
                RedisMsg oRedisMsg;
                eCodecStatus = pChannel->m_pImpl->Fetch(oBuff);
                if (CODEC_STATUS_OK == eCodecStatus)
                {
                    m_pLabor->GetActorBuilder()->OnMessage(pChannel, oBuff);
                }
                else
                {
                    break;
                }
            }
        }
            break;
        default:
            for (int i = 0; ; ++i)
            {
                MsgHead oMsgHead;
                MsgBody oMsgBody;
                eCodecStatus = pChannel->m_pImpl->Fetch(oMsgHead, oMsgBody);
                if (CODEC_STATUS_OK == eCodecStatus)
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
                }
                else
                {
                    break;
                }
            }
    }

    switch (eCodecStatus)
    {
        case CODEC_STATUS_PAUSE: 
            return(true);
        case CODEC_STATUS_WANT_WRITE: 
            return(SendTo(pChannel));
        case CODEC_STATUS_WANT_READ: 
            RemoveIoWriteEvent(pChannel);
            return(true);
        default: // 编解码出错或连接关闭或连接中断
            if (CODEC_STATUS_INVALID == eCodecStatus && !pChannel->IsClient())
            {
                if (pChannel->m_pImpl->AutoSwitchCodec())
                {
                    return(DataFetchAndHandle(pChannel));
                }
            }
            if (CHANNEL_STATUS_ESTABLISHED != pChannel->m_pImpl->GetChannelStatus())
            {
                if (pChannel->IsClient())
                {
                    m_pSessionNode->NodeFailed(pChannel->GetIdentify());
                }
                auto& listUncompletedStep = pChannel->m_pImpl->GetPipelineStepSeq();
                for (auto it = listUncompletedStep.begin();
                        it != listUncompletedStep.end(); ++it)
                {
                    m_pLabor->GetActorBuilder()->OnError(pChannel, *it,
                            pChannel->m_pImpl->GetErrno(), pChannel->m_pImpl->GetErrMsg());
                }
            }
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
        if (CODEC_NEBULA != iCodec && m_pLabor->WithSsl())
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
            std::shared_ptr<Step> pStepConnectWorker = m_pLabor->GetActorBuilder()->MakeSharedStep(
                    nullptr, "neb::StepConnectWorker", pChannel, pChannel->m_pImpl->GetRemoteWorkerIndex());
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
    else
    {
        if (CHANNEL_STATUS_CLOSED != pChannel->m_pImpl->GetChannelStatus())
        {
            pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
        }
    }

    if (pChannel->IsClient() && pChannel->m_pImpl->GetMsgNum() == 0)
    {
        m_pSessionNode->NodeRecover(pChannel->GetIdentify());
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
    //ev_tstamp after = pChannel->m_pImpl->GetActiveTime() - ev_now(m_loop) + m_pLabor->GetNodeInfo().dIoTimeout;
    ev_tstamp after = pChannel->m_pImpl->GetActiveTime() - ev_now(m_loop) + pChannel->m_pImpl->GetKeepAlive();
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

void Dispatcher::EventRun()
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

bool Dispatcher::SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    if (m_pLabor->GetLaborType() == Labor::LABOR_MANAGER)
    {
        return(Broadcast("BEACON", iCmd, uiSeq, oMsgBody, CODEC_NEBULA));
    }
    else
    {
        auto pChannel = ((Worker*)m_pLabor)->GetManagerControlChannel();
        if (pChannel == nullptr)
        {
            LOG4_ERROR("no connected channel to manager!");
            return(false);
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

std::shared_ptr<SocketChannel> Dispatcher::StressSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    size_t iPosIpPortSeparator = strIdentify.rfind(":");
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(nullptr);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(nullptr);
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
        return(nullptr);
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

    /* no address succeeded */
    if (pAddrCurrent == NULL)
    {
        LOG4_ERROR("Could not connect to \"%s:%s\"", strHost.c_str(), strPort.c_str());
        freeaddrinfo(pAddrResult);
        return(nullptr);
    }

    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    int iKeepAlive = 1;
    int iKeepIdle = 60;
    int iKeepInterval = 5;
    int iKeepCount = 3;
    int iTcpNoDelay = 1;
    int iTcpQuickAck = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    setsockopt(iFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval));
    setsockopt(iFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount));
    setsockopt(iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay));
    setsockopt(iFd, IPPROTO_TCP, TCP_QUICKACK, (void*)&iTcpQuickAck, sizeof(iTcpQuickAck));
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iFd, eCodecType);
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        freeaddrinfo(pAddrResult);  /* no longer needed */
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
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
        return(pChannel);
    }
    else
    {
        freeaddrinfo(pAddrResult);
        close(iFd);
        return(nullptr);
    }
}

bool Dispatcher::SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    if (m_pLabor->GetLaborType() == Labor::LABOR_MANAGER)
    {
        LOG4_ERROR("this function can not be called by manager process!");
        return(false);
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
        std::unordered_set<std::shared_ptr<SocketChannel>>::iterator channel_iter;
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
        std::unordered_set<std::shared_ptr<SocketChannel>> setChannel;
        setChannel.insert(pChannel);
        m_mapNamedSocketChannel.insert(std::make_pair(strIdentify, std::move(setChannel)));
    }
    else
    {
        named_iter->second.insert(pChannel);
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

bool Dispatcher::AddEvent(ev_idle* idle_watcher, idle_callback pFunc)
{
    if (NULL == idle_watcher)
    {
        return(false);
    }
    ev_idle_init (idle_watcher, pFunc);
    ev_idle_start (m_loop, idle_watcher);
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
    Codec::AddAutoSwitchCodecType(CODEC_HTTP);
    Codec::AddAutoSwitchCodecType(CODEC_PROTO);
    Codec::AddAutoSwitchCodecType(CODEC_PRIVATE);
    return(true);
}

void Dispatcher::Destroy()
{
    m_mapSocketChannel.clear();
    m_mapNamedSocketChannel.clear();
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
        bool bInitResult = pChannel->Init(eCodecType, bIsClient);
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

    bool bCloseResult = pChannel->m_pImpl->Close();
    if (bCloseResult)
    {
        LOG4_DEBUG("%s disconnect, fd %d, channel_seq %u, identify %s",
                pChannel->m_pImpl->GetRemoteAddr().c_str(),
                pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence(),
                pChannel->m_pImpl->GetIdentify().c_str());
        if (bChannelNotice)
        {
            m_pLabor->GetActorBuilder()->ChannelNotice(pChannel, pChannel->m_pImpl->GetIdentify(), pChannel->m_pImpl->GetClientData());
        }
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
    if (NULL == io_watcher || pChannel->GetFd() < 0)
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
    if (NULL == io_watcher || pChannel->GetFd() < 0)
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
    if (NULL == io_watcher || pChannel->GetFd() < 0)
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
            AddIoTimeout(pChannel, 1.0);     // 初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
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
