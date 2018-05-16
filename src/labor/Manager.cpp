/*******************************************************************************
 * Project:  Nebula
 * @file     Manager.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "util/proctitle_helper.h"
#include "util/process_helper.h"

#ifdef __cplusplus
}
#endif

#include "Manager.hpp"
#include "Worker.hpp"

namespace neb
{

void Manager::SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        if (SIGCHLD == watcher->signum)
        {
            pManager->OnChildTerminated(watcher);
        }
        else
        {
            pManager->OnManagerTerminated(watcher);
        }
    }
}

void Manager::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        pManager->CheckWorker();
        pManager->ReportToBeacon();
    }
}

void Manager::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = (SocketChannel*)watcher->data;
        Manager* pManager = (Manager*)pChannel->m_pImpl->m_pLabor;
        if (revents & EV_READ)
        {
            pManager->OnIoRead(pChannel->shared_from_this());
        }
        else if (revents & EV_WRITE)
        {
            pManager->OnIoWrite(pChannel->shared_from_this());
        }
        else if (revents & EV_ERROR)
        {
            pManager->OnIoError(pChannel->shared_from_this());
        }
        if (CHANNEL_STATUS_ABORT == pChannel->m_pImpl->GetChannelStatus())
        {
            watcher->data = NULL;
        }
    }
}

void Manager::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        SocketChannel* pChannel = (SocketChannel*)watcher->data;
        Manager* pManager = (Manager*)pChannel->m_pImpl->m_pLabor;
        pManager->OnIoTimeout(pChannel->shared_from_this());
    }
}

void Manager::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)(watcher->data);
        pManager->ReportToBeacon();
        pManager->CheckWorker();
        pManager->RefreshServer();
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Manager::ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        tagClientConnWatcherData* pData = (tagClientConnWatcherData*)watcher->data;
        Manager* pManager = (Manager*)pData->pLabor;
        pManager->OnClientConnFrequencyTimeout(pData, watcher);
    }
}

Manager::Manager(const std::string& strConfFile)
    : m_uiSequence(0), m_loop(NULL), m_pPeriodicTaskWatcher(NULL)
{
    if (strConfFile == "")
    {
        std::cerr << "error: no config file!" << std::endl;
        exit(1);
    }

    m_stManagerInfo.strConfFile = strConfFile;
    if (!GetConf())
    {
        std::cerr << "GetConf() error!" << std::endl;
        exit(-1);
    }
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    daemonize(m_oCurrentConf("server_name").c_str());
    Init();
    m_stManagerInfo.iWorkerBeat = (gc_iBeatInterval * 2) + 1;
    CreateEvents();
    CreateWorker();
    RegisterToBeacon();
}

Manager::~Manager()
{
    Destroy();
}

bool Manager::OnManagerTerminated(struct ev_signal* watcher)
{
    LOG4_WARNING("%s terminated by signal %d!", m_oCurrentConf("server_name").c_str(), watcher->signum);
    ev_break (m_loop, EVBREAK_ALL);
    exit(-1);
}

bool Manager::OnChildTerminated(struct ev_signal* watcher)
{
    pid_t   iPid = 0;
    int     iStatus = 0;
    int     iReturnCode = 0;
    //WUNTRACED
    while((iPid = waitpid(-1, &iStatus, WNOHANG)) > 0)
    {
        if (WIFEXITED(iStatus))
        {
            iReturnCode = WEXITSTATUS(iStatus);
        }
        else if (WIFSIGNALED(iStatus))
        {
            iReturnCode = WTERMSIG(iStatus);
        }
        else if (WIFSTOPPED(iStatus))
        {
            iReturnCode = WSTOPSIG(iStatus);
        }

        LOG4_FATAL("error %d: process %d exit and sent signal %d with code %d!",
                        iStatus, iPid, watcher->signum, iReturnCode);
        RestartWorker(iPid);
    }
    return(true);
}

bool Manager::OnIoRead(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d]", pChannel->m_pImpl->GetFd());
    if (pChannel->m_pImpl->GetFd() == m_stManagerInfo.iS2SListenFd)
    {
        return(AcceptServerConn(pChannel->m_pImpl->GetFd()));
    }
    else if (m_stManagerInfo.iC2SListenFd > 2 && pChannel->m_pImpl->GetFd() == m_stManagerInfo.iC2SListenFd)
    {
        return(FdTransfer(pChannel->m_pImpl->GetFd()));
    }
    else
    {
        return(DataRecvAndHandle(pChannel));
    }
}

bool Manager::FdTransfer(int iFd)
{
    char szIpAddr[16] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        return(false);
    }
    strncpy(szIpAddr, inet_ntoa(stClientAddr.sin_addr), 16);
    /* tcp连接检测 */
//    int iKeepAlive = 1;
//    int iKeepIdle = 1000;
//    int iKeepInterval = 10;
//    int iKeepCount = 3;
//    int iTcpNoDelay = 1;
//    if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
//    {
//        LOG4_WARNING("fail to set SO_KEEPALIVE");
//    }
//    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
//    {
//        LOG4_WARNING("fail to set TCP_KEEPIDLE");
//    }
//    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
//    {
//        LOG4_WARNING("fail to set TCP_KEEPINTVL");
//    }
//    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
//    {
//        LOG4_WARNING("fail to set TCP_KEEPCNT");
//    }
//    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
//    {
//        LOG4_WARNING("fail to set TCP_NODELAY");
//    }

    auto iter = m_mapClientConnFrequency.find(stClientAddr.sin_addr.s_addr);
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::pair<in_addr_t, uint32>(stClientAddr.sin_addr.s_addr, 1));
        AddClientConnFrequencyTimeout(stClientAddr.sin_addr.s_addr, m_stManagerInfo.dAddrStatInterval);
    }
    else
    {
        iter->second++;
        if (m_stManagerInfo.iC2SListenFd > 2 && iter->second > (uint32)m_stManagerInfo.iAddrPermitNum)
        {
            LOG4_WARNING("client addr %d had been connected more than %u times in %f seconds, it's not permitted",
                            stClientAddr.sin_addr.s_addr, m_stManagerInfo.iAddrPermitNum, m_stManagerInfo.dAddrStatInterval);
            return(false);
        }
    }

    std::pair<int, int> worker_pid_fd = GetMinLoadWorkerDataFd();
    if (worker_pid_fd.second > 0)
    {
        LOG4_DEBUG("send new fd %d to worker communication fd %d",
                        iAcceptFd, worker_pid_fd.second);
        int iCodec = m_stManagerInfo.eCodec;
        //int iErrno = send_fd(worker_pid_fd.second, iAcceptFd);
        //int iErrno = send_fd_with_attr(worker_pid_fd.second, iAcceptFd, szIpAddr, 16, iCodec);
        int iErrno = SocketChannel::SendChannelFd(worker_pid_fd.second, iAcceptFd, iCodec, m_pLogger);
        if (iErrno == 0)
        {
            AddWorkerLoad(worker_pid_fd.first);
        }
        else
        {
            LOG4_ERROR("error %d: %s", iErrno, strerror_r(iErrno, m_szErrBuff, 1024));
        }
        close(iAcceptFd);
        return(true);
    }
    return(false);
}

bool Manager::AcceptServerConn(int iFd)
{
    struct sockaddr_in stClientAddr;
    socklen_t clientAddrSize = sizeof(stClientAddr);
    int iAcceptFd = accept(iFd, (struct sockaddr*) &stClientAddr, &clientAddrSize);
    if (iAcceptFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
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
        std::shared_ptr<SocketChannel> pChannel = CreateChannel(iAcceptFd, CODEC_NEBULA);
        if (NULL != pChannel)
        {
            AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            AddIoReadEvent(pChannel);
        }
    }
    return(false);
}

bool Manager::DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_DEBUG("fd %d, seq %llu", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    E_CODEC_STATUS eCodecStatus;
    if (CODEC_HTTP == pChannel->m_pImpl->GetCodecType())   // Manager只有pb协议编解码
    {
//        HttpMsg oHttpMsg;
//        eCodecStatus = pChannel->m_pImpl->Recv(oHttpMsg);
        DiscardSocketChannel(pChannel);
        return(false);
    }
    else
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        eCodecStatus = pChannel->m_pImpl->Recv(oMsgHead, oMsgBody);
        while (CODEC_STATUS_OK == eCodecStatus)
        {
            auto worker_fd_iter = m_mapWorkerFdPid.find(pChannel->m_pImpl->GetFd());
            if (worker_fd_iter == m_mapWorkerFdPid.end())   // 其他Server发过来要将连接传送到某个指定Worker进程信息
            {
                if (m_pSessionNode->IsNodeType(pChannel->m_pImpl->GetIdentify(), "BEACON"))       // 与beacon连接
                {
                    OnBeaconData(pChannel, oMsgHead, oMsgBody);
                }
                else    // 不是与beacon连接
                {
                    OnDataAndTransferFd(pChannel, oMsgHead, oMsgBody);
                }
            }
            else    // Worker进程发过来的消息
            {
                OnWorkerData(pChannel, oMsgHead, oMsgBody);
            }
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->m_pImpl->Fetch(oMsgHead, oMsgBody);
        }

        if (CODEC_STATUS_PAUSE == eCodecStatus)
        {
            return(true);
        }
        else    // 编解码出错或连接关闭或连接中断
        {
            DiscardSocketChannel(pChannel);
            return(false);
        }
    }
}

bool Manager::OnIoWrite(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%d", pChannel->m_pImpl->GetFd());
    if (CHANNEL_STATUS_TRY_CONNECT == pChannel->m_pImpl->GetChannelStatus())  // connect之后的第一个写事件
    {
        MsgBody oMsgBody;
        oMsgBody.set_data(std::to_string(pChannel->m_pImpl->m_unRemoteWorkerIdx));
        LOG4_DEBUG("send after connect, oMsgBody.ByteSize() = %d", oMsgBody.ByteSize());
        SendTo(pChannel, CMD_REQ_CONNECT_TO_WORKER, GetSequence(), oMsgBody);
        return(true);
    }
    else
    {
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
    }
    return(true);
}

bool Manager::OnIoError(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE(" ");
    return(false);
}

bool Manager::OnIoTimeout(std::shared_ptr<SocketChannel> pChannel)
{
    ev_tstamp after = pChannel->m_pImpl->GetActiveTime() - ev_now(m_loop) + m_stManagerInfo.dIoTimeout;
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer* watcher = pChannel->m_pImpl->MutableTimerWatcher();
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // IO已超时，关闭文件描述符并清理相关资源
    {
        LOG4_TRACE(" ");
        DiscardSocketChannel(pChannel);
        return(false);
    }
}

bool Manager::OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher)
{
    bool bRes = false;
    auto iter = m_mapClientConnFrequency.find(pData->iAddr);
    if (iter == m_mapClientConnFrequency.end())
    {
        bRes = false;
    }
    else
    {
        m_mapClientConnFrequency.erase(iter);
        bRes = true;
    }

    ev_timer_stop(m_loop, watcher);
    pData->pLabor = NULL;
    delete pData;
    watcher->data = NULL;
    delete watcher;
    watcher = NULL;
    return(bRes);
}

void Manager::Run()
{
    LOG4_TRACE(" ");
    ev_run (m_loop, 0);
}

bool Manager::InitLogger(const CJsonObject& oJsonConf)
{
    int iLogLevel = Logger::INFO;
    int iNetLogLevel = Logger::INFO;
    if (nullptr != m_pLogger)  // 已经被初始化过，只修改日志级别
    {
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
        std::string strLoggingHost;
        std::string strLogname = oJsonConf("log_path") + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_stManagerInfo.strHostForServer << ":" << m_stManagerInfo.iPortForServer;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_level", iLogLevel);
        m_pLogger = std::make_shared<NetLogger>(strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, this);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        LOG4_NOTICE("%s program begin, and work path %s...", oJsonConf("server_name").c_str(), m_stManagerInfo.strWorkPath.c_str());
        return(true);
    }
}

bool Manager::SetProcessName(const CJsonObject& oJsonConf)
{
    ngx_setproctitle(oJsonConf("server_name").c_str());
    return(true);
}

bool Manager::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
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
        return(false);
    }
    return(true);
}

bool Manager::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("cmd[%u], seq[%u]", iCmd, uiSeq);
    E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
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
        return(false);
    }
    return(true);
}

bool Manager::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        E_CODEC_STATUS eStatus = named_iter->second->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus)
        {
            // do not need to RemoveIoWriteEvent(pSocketChannel);  because had not been AddIoWriteEvent(pSocketChannel).
            return(true);
        }
        else if (CODEC_STATUS_PAUSE == eStatus)
        {
            AddIoWriteEvent(named_iter->second);
            return(true);
        }
        return(false);
    }
}

bool Manager::SendPolling(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool Manager::SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    if (std::string("LOGGER") != strNodeType)
    {
        LOG4_ERROR("SendOriented() only support node_type \"LOGGER\" in manager process!");
        return(false);
    }

    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        return(SendTo(strOnlineNode, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

bool Manager::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
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
}

bool Manager::Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::unordered_set<std::string> setOnlineNodes;
    if (m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
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

bool Manager::AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);

    x_sock_set_block(iFd, 0);
    int reuse = 1;
    ::setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    std::shared_ptr<SocketChannel> pChannel = CreateChannel(iFd, CODEC_NEBULA);
    if (nullptr != pChannel)
    {
        int iKeepAlive = 1;
        int iKeepIdle = 60;
        int iKeepInterval = 5;
        int iKeepCount = 3;
        int iTcpNoDelay = 1;
        if (setsockopt(iFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            LOG4_WARNING("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPIDLE");
        }
        if (setsockopt(iFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPINTVL");
        }
        if (setsockopt(iFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARNING("fail to set TCP_KEEPCNT");
        }
        if (setsockopt(iFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARNING("fail to set TCP_NODELAY");
        }
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
    close(iFd);
    return(false);
}

bool Manager::GetConf()
{
    char szFilePath[256] = {0};
    //char szFileName[256] = {0};
    if (m_stManagerInfo.strWorkPath.length() == 0)
    {
        if (getcwd(szFilePath, sizeof(szFilePath)))
        {
            m_stManagerInfo.strWorkPath = szFilePath;
        }
        else
        {
            std::cerr << "failed to open work dir: " << m_stManagerInfo.strWorkPath << std::endl;
            return(false);
        }
    }
    m_oLastConf = m_oCurrentConf;
    //snprintf(szFileName, sizeof(szFileName), "%s/%s", m_strWorkPath.c_str(), m_strConfFile.c_str());
    std::ifstream fin(m_stManagerInfo.strConfFile.c_str());
    if (fin.good())
    {
        std::stringstream ssContent;
        ssContent << fin.rdbuf();
        if (!m_oCurrentConf.Parse(ssContent.str()))
        {
            ssContent.str("");
            fin.close();
            m_oCurrentConf = m_oLastConf;
            std::cerr << "failed to parse json file " << m_stManagerInfo.strConfFile << std::endl;
            return(false);
        }
        ssContent.str("");
        fin.close();
    }
    else
    {
        std::cerr << "failed to open file " << m_stManagerInfo.strConfFile << std::endl;
        return(false);
    }

    if (m_oLastConf.ToString() != m_oCurrentConf.ToString())
    {
        m_oCurrentConf.Get("io_timeout", m_stManagerInfo.dIoTimeout);
        if (m_oLastConf.ToString().length() == 0)
        {
            m_stManagerInfo.uiWorkerNum = strtoul(m_oCurrentConf("process_num").c_str(), NULL, 10);
            m_oCurrentConf.Get("node_type", m_stManagerInfo.strNodeType);
            m_oCurrentConf.Get("host", m_stManagerInfo.strHostForServer);
            m_oCurrentConf.Get("port", m_stManagerInfo.iPortForServer);
            m_oCurrentConf.Get("access_host", m_stManagerInfo.strHostForClient);
            m_oCurrentConf.Get("access_port", m_stManagerInfo.iPortForClient);
            m_oCurrentConf.Get("gateway", m_stManagerInfo.strGateway);
            m_oCurrentConf.Get("gateway_port", m_stManagerInfo.iGatewayPort);
            m_stManagerInfo.strNodeIdentify = m_stManagerInfo.strHostForServer + std::string(":") + std::to_string(m_stManagerInfo.iPortForServer);
        }
        int32 iCodec;
        if (m_oCurrentConf.Get("access_codec", iCodec))
        {
            m_stManagerInfo.eCodec = E_CODEC_TYPE(iCodec);
        }
        m_oCurrentConf["permission"]["addr_permit"].Get("stat_interval", m_stManagerInfo.dAddrStatInterval);
        m_oCurrentConf["permission"]["addr_permit"].Get("permit_num", m_stManagerInfo.iAddrPermitNum);
    }
    return(true);
}

bool Manager::Init()
{
    InitLogger(m_oCurrentConf);
    m_pSessionNode = std::make_unique<SessionNode>();

    socklen_t addressLen = 0;
    int queueLen = 100;
    int reuse = 1;
    int timeout = 1;

    if (m_stManagerInfo.strHostForClient.size() > 0 && m_stManagerInfo.iPortForClient > 0)
    {
        // 接入节点才需要监听客户端连接
        struct sockaddr_in stAddrOuter;
        struct sockaddr *pAddrOuter;
        stAddrOuter.sin_family = AF_INET;
        stAddrOuter.sin_port = htons(m_stManagerInfo.iPortForClient);
        stAddrOuter.sin_addr.s_addr = inet_addr(m_stManagerInfo.strHostForClient.c_str());
        pAddrOuter = (struct sockaddr*)&stAddrOuter;
        addressLen = sizeof(struct sockaddr);
        m_stManagerInfo.iC2SListenFd = socket(pAddrOuter->sa_family, SOCK_STREAM, 0);
        if (m_stManagerInfo.iC2SListenFd < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
            int iErrno = errno;
            exit(iErrno);
        }
        reuse = 1;
        timeout = 1;
        ::setsockopt(m_stManagerInfo.iC2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        ::setsockopt(m_stManagerInfo.iC2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
        if (bind(m_stManagerInfo.iC2SListenFd, pAddrOuter, addressLen) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
            close(m_stManagerInfo.iC2SListenFd);
            m_stManagerInfo.iC2SListenFd = -1;
            int iErrno = errno;
            exit(iErrno);
        }
        if (listen(m_stManagerInfo.iC2SListenFd, queueLen) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
            close(m_stManagerInfo.iC2SListenFd);
            m_stManagerInfo.iC2SListenFd = -1;
            int iErrno = errno;
            exit(iErrno);
        }
    }

    struct sockaddr_in stAddrInner;
    struct sockaddr *pAddrInner;
    stAddrInner.sin_family = AF_INET;
    stAddrInner.sin_port = htons(m_stManagerInfo.iPortForServer);
    stAddrInner.sin_addr.s_addr = inet_addr(m_stManagerInfo.strHostForServer.c_str());
    pAddrInner = (struct sockaddr*)&stAddrInner;
    addressLen = sizeof(struct sockaddr);
    m_stManagerInfo.iS2SListenFd = socket(pAddrInner->sa_family, SOCK_STREAM, 0);
    if (m_stManagerInfo.iS2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    reuse = 1;
    timeout = 1;
    ::setsockopt(m_stManagerInfo.iS2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ::setsockopt(m_stManagerInfo.iS2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
    if (bind(m_stManagerInfo.iS2SListenFd, pAddrInner, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_stManagerInfo.iS2SListenFd);
        m_stManagerInfo.iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_stManagerInfo.iS2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_stManagerInfo.iS2SListenFd);
        m_stManagerInfo.iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }

    // 创建到beacon的连接信息
    for (int i = 0; i < m_oCurrentConf["beacon"].GetArraySize(); ++i)
    {
        std::string strIdentify = m_oCurrentConf["beacon"][i]("host") + std::string(":")
            + m_oCurrentConf["beacon"][i]("port") + std::string(".1");     // BeaconServer只有一个Worker
        AddNodeIdentify(std::string("BEACON"), strIdentify);
    }

    return(true);
}

void Manager::Destroy()
{
    LOG4_TRACE(" ");
    m_mapWorker.clear();
    m_mapWorkerFdPid.clear();
    m_mapWorkerRestartNum.clear();
    for (auto iter = m_mapSocketChannel.begin(); iter != m_mapSocketChannel.end(); ++iter)
    {
        DiscardSocketChannel(iter->second);
    }
    m_mapSocketChannel.clear();
    m_mapClientConnFrequency.clear();
    m_vecFreeWorkerIdx.clear();
    if (m_pPeriodicTaskWatcher != NULL)
    {
        free(m_pPeriodicTaskWatcher);
    }
    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }
}

bool Manager::CreateEvents()
{
    LOG4_TRACE(" ");
    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
    if (m_loop == NULL)
    {
        return(false);
    }
    std::shared_ptr<SocketChannel> pChannelListen = NULL;
    if (m_stManagerInfo.iC2SListenFd > 2)
    {
        LOG4_DEBUG("C2SListenFd[%d]", m_stManagerInfo.iC2SListenFd);
        pChannelListen = CreateChannel(m_stManagerInfo.iC2SListenFd, m_stManagerInfo.eCodec);
        pChannelListen->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
        AddIoReadEvent(pChannelListen);
    }
    LOG4_DEBUG("S2SListenFd[%d]", m_stManagerInfo.iS2SListenFd);
    pChannelListen = CreateChannel(m_stManagerInfo.iS2SListenFd, CODEC_NEBULA);
    pChannelListen->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    AddIoReadEvent(pChannelListen);

    ev_signal* child_signal_watcher = new ev_signal();
    ev_signal_init (child_signal_watcher, SignalCallback, SIGCHLD);
    child_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, child_signal_watcher);

    /*
    ev_signal* int_signal_watcher = new ev_signal();
    ev_signal_init (int_signal_watcher, SignalCallback, SIGINT);
    int_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, int_signal_watcher);
    */

    ev_signal* ill_signal_watcher = new ev_signal();
    ev_signal_init (ill_signal_watcher, SignalCallback, SIGILL);
    ill_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, ill_signal_watcher);

    ev_signal* bus_signal_watcher = new ev_signal();
    ev_signal_init (bus_signal_watcher, SignalCallback, SIGBUS);
    bus_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, bus_signal_watcher);

    ev_signal* fpe_signal_watcher = new ev_signal();
    ev_signal_init (fpe_signal_watcher, SignalCallback, SIGFPE);
    fpe_signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, fpe_signal_watcher);

    AddPeriodicTaskEvent();
    // 注册idle事件在Server空闲时会导致CPU占用过高，暂时弃用之，改用定时器实现
//    ev_idle* idle_watcher = new ev_idle();
//    ev_idle_init (idle_watcher, IdleCallback);
//    idle_watcher->data = (void*)this;
//    ev_idle_start (m_loop, idle_watcher);

    return(true);
}

bool Manager::AddPeriodicTaskEvent()
{
    LOG4_TRACE(" ");
    m_pPeriodicTaskWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (m_pPeriodicTaskWatcher == NULL)
    {
        LOG4_ERROR("new timeout_watcher error!");
        return(false);
    }
    ev_timer_init (m_pPeriodicTaskWatcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    m_pPeriodicTaskWatcher->data = (void*)this;
    ev_timer_start (m_loop, m_pPeriodicTaskWatcher);
    return(true);
}

bool Manager::AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel)
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

bool Manager::AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
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

bool Manager::RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel)
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

bool Manager::DelEvents(ev_io* io_watcher)
{
    if (NULL == io_watcher)
    {
        return(false);
    }
    LOG4_TRACE("fd %d", io_watcher->fd);
    ev_io_stop (m_loop, io_watcher);
    return(true);
}

bool Manager::DelEvents(ev_timer* timer_watcher)
{
    if (NULL == timer_watcher)
    {
        return(false);
    }
    ev_timer_stop (m_loop, timer_watcher);
    return(true);
}

void Manager::CreateWorker()
{
    LOG4_TRACE(" ");
    int iPid = 0;

    for (unsigned int i = 1; i <= m_stManagerInfo.uiWorkerNum; ++i)
    {
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        }

        iPid = fork();
        if (iPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            close(m_stManagerInfo.iS2SListenFd);
            if (m_stManagerInfo.iC2SListenFd > 2)
            {
                close(m_stManagerInfo.iC2SListenFd);
            }
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker oWorker(m_stManagerInfo.strWorkPath, iControlFds[1], iDataFds[1], i, m_oCurrentConf);
            oWorker.Run();
            exit(-2);
        }
        else if (iPid > 0)   // 父进程
        {
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            tagWorkerAttr stWorkerAttr;
            stWorkerAttr.iWorkerIndex = i;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            stWorkerAttr.dBeatTime = ev_now(m_loop);
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iPid));
            std::shared_ptr<SocketChannel> pChannelData = CreateChannel(iControlFds[0], CODEC_NEBULA);
            std::shared_ptr<SocketChannel> pChannelControl = CreateChannel(iDataFds[0], CODEC_NEBULA);
            pChannelData->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            pChannelControl->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            AddIoReadEvent(pChannelData);
            AddIoReadEvent(pChannelControl);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        }
    }
}

bool Manager::CheckWorker()
{
    LOG4_TRACE(" ");

    for (auto worker_iter = m_mapWorker.begin();
                    worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        LOG4_TRACE("now %lf, worker_beat_time %lf, worker_beat %d",
                        ev_now(m_loop), worker_iter->second.dBeatTime, m_stManagerInfo.iWorkerBeat);
        if ((ev_now(m_loop) - worker_iter->second.dBeatTime) > m_stManagerInfo.iWorkerBeat)
        {
            LOG4_INFO("worker_%d pid %d is unresponsive, "
                            "terminate it.", worker_iter->second.iWorkerIndex, worker_iter->first);
            kill(worker_iter->first, SIGKILL); //SIGINT);
//            RestartWorker(worker_iter->first);
        }
    }
    return(true);
}

bool Manager::RestartWorker(int iDeathPid)
{
    LOG4_DEBUG("%d", iDeathPid);
    int iNewPid = 0;
    char errMsg[1024] = {0};
    auto worker_iter = m_mapWorker.find(iDeathPid);
    if (worker_iter != m_mapWorker.end())
    {
        LOG4_TRACE("restart worker %d, close control fd %d and data fd %d first.",
                        worker_iter->second.iWorkerIndex, worker_iter->second.iControlFd, worker_iter->second.iDataFd);
        int iWorkerIndex = worker_iter->second.iWorkerIndex;
        auto fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iControlFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iDataFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        auto channel_iter = m_mapSocketChannel.find(worker_iter->second.iControlFd);
        if (channel_iter != m_mapSocketChannel.end())
        {
            DiscardSocketChannel(channel_iter->second);
        }
        channel_iter = m_mapSocketChannel.find(worker_iter->second.iDataFd);
        if (channel_iter != m_mapSocketChannel.end())
        {
            DiscardSocketChannel(channel_iter->second);
        }
        m_mapWorker.erase(worker_iter);

        auto restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
        if (restart_num_iter != m_mapWorkerRestartNum.end())
        {
            LOG4_INFO("worker %d had been restarted %d times!", iWorkerIndex, restart_num_iter->second);
        }

        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }

        iNewPid = fork();
        if (iNewPid == 0)   // 子进程
        {
            ev_loop_destroy(m_loop);
            close(m_stManagerInfo.iS2SListenFd);
            if (m_stManagerInfo.iC2SListenFd > 2)
            {
                close(m_stManagerInfo.iC2SListenFd);
            }
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker oWorker(m_stManagerInfo.strWorkPath, iControlFds[1], iDataFds[1], iWorkerIndex, m_oCurrentConf);
            oWorker.Run();
            exit(-2);   // 子进程worker没有正常运行
        }
        else if (iNewPid > 0)   // 父进程
        {
            LOG4_INFO("worker %d restart successfully", iWorkerIndex);
            ev_loop_fork(m_loop);
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            tagWorkerAttr stWorkerAttr;
            stWorkerAttr.iWorkerIndex = iWorkerIndex;
            stWorkerAttr.iControlFd = iControlFds[0];
            stWorkerAttr.iDataFd = iDataFds[0];
            stWorkerAttr.dBeatTime = ev_now(m_loop);
            LOG4_TRACE("m_mapWorker insert (iNewPid %d, worker_index %d)", iNewPid, iWorkerIndex);
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iNewPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iNewPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iNewPid));
            std::shared_ptr<SocketChannel> pChannelData = CreateChannel(iControlFds[0], CODEC_NEBULA);
            std::shared_ptr<SocketChannel> pChannelControl = CreateChannel(iDataFds[0], CODEC_NEBULA);
            pChannelData->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            pChannelControl->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            AddIoReadEvent(pChannelData);
            AddIoReadEvent(pChannelControl);
            restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
            if (restart_num_iter == m_mapWorkerRestartNum.end())
            {
                m_mapWorkerRestartNum.insert(std::pair<int, int>(iWorkerIndex, 1));
            }
            else
            {
                restart_num_iter->second++;
            }
            RegisterToBeacon();     // 重启Worker进程后向Beacon重发注册请求，以获得beacon下发其他节点的信息
            return(true);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
    }
    return(false);
}

std::pair<int, int> Manager::GetMinLoadWorkerDataFd()
{
    LOG4_TRACE(" ");
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_fd;
    for (auto iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
    {
       if (iter == m_mapWorker.begin())
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
       }
       else if (iter->second.iLoad < iMinLoad)
       {
           iMinLoadWorkerFd = iter->second.iDataFd;
           iMinLoad = iter->second.iLoad;
           worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
       }
    }
    return(worker_pid_fd);
}

bool Manager::SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    std::unordered_map<int, std::shared_ptr<SocketChannel>>::iterator worker_conn_iter;
    for (auto worker_iter = m_mapWorker.begin(); worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        worker_conn_iter = m_mapSocketChannel.find(worker_iter->second.iControlFd);
        if (worker_conn_iter != m_mapSocketChannel.end())
        {
            E_CODEC_STATUS eCodecStatus = worker_conn_iter->second->m_pImpl->Send(iCmd, uiSeq, oMsgBody);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                RemoveIoWriteEvent(worker_conn_iter->second);
            }
            else if (CODEC_STATUS_PAUSE == eCodecStatus)
            {
                AddIoWriteEvent(worker_conn_iter->second);
            }
            else
            {
                DiscardSocketChannel(worker_conn_iter->second);
            }
        }
    }
    return(true);
}

void Manager::RefreshServer()
{
    LOG4_TRACE(" ");
    if (GetConf())
    {
        if (m_oLastConf("log_level") != m_oCurrentConf("log_level") || m_oLastConf("net_log_level") != m_oCurrentConf("net_log_level") )
        {
            int iLogLevel = Logger::INFO;
            int iNetLogLevel = Logger::INFO;
            m_pLogger->SetLogLevel(iLogLevel);
            m_pLogger->SetNetLogLevel(iNetLogLevel);
            MsgBody oMsgBody;
            LogLevel oLogLevel;
            oLogLevel.set_log_level(iLogLevel);
            oLogLevel.set_net_log_level(iNetLogLevel);
            oMsgBody.set_data(oLogLevel.SerializeAsString());
            SendToWorker(CMD_REQ_SET_LOG_LEVEL, GetSequence(), oMsgBody);
        }

        // 更新动态库配置或重新加载动态库
        if (m_oLastConf["dynamic_loading"].ToString() != m_oCurrentConf["dynamic_loading"].ToString())
        {
            MsgBody oMsgBody;
            oMsgBody.set_data(m_oCurrentConf["dynamic_loading"].ToString());
            SendToWorker(CMD_REQ_RELOAD_SO, GetSequence(), oMsgBody);
        }
    }
    else
    {
        LOG4_ERROR("GetConf() error, please check the config file.", "");
    }
}

bool Manager::RegisterToBeacon()
{
    if (std::string("BEACON") == m_stManagerInfo.strNodeType)
    {
        return(true);
    }
    LOG4_TRACE(" ");
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgBody oMsgBody;
    CJsonObject oReportData;
    CJsonObject oMember;
    oReportData.Add("node_type", m_stManagerInfo.strNodeType);
    oReportData.Add("node_id", m_stManagerInfo.uiNodeId);
    oReportData.Add("node_ip", m_stManagerInfo.strHostForServer);
    oReportData.Add("node_port", m_stManagerInfo.iPortForServer);
    if (m_stManagerInfo.strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_stManagerInfo.strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_stManagerInfo.strHostForClient);
    }
    if (m_stManagerInfo.iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_stManagerInfo.iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_stManagerInfo.iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    auto worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second.iLoad;
        iConnect += worker_iter->second.iConnect;
        iRecvNum += worker_iter->second.iRecvNum;
        iRecvByte += worker_iter->second.iRecvByte;
        iSendNum += worker_iter->second.iSendNum;
        iSendByte += worker_iter->second.iSendByte;
        iClientNum += worker_iter->second.iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second.iLoad);
        oMember.Add("connect", worker_iter->second.iConnect);
        oMember.Add("recv_num", worker_iter->second.iRecvNum);
        oMember.Add("recv_byte", worker_iter->second.iRecvByte);
        oMember.Add("send_num", worker_iter->second.iSendNum);
        oMember.Add("send_byte", worker_iter->second.iSendByte);
        oMember.Add("client", worker_iter->second.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_data(oReportData.ToString());
    Broadcast("BEACON", CMD_REQ_NODE_REGISTER, GetSequence(), oMsgBody);
    return(true);
}

/**
 * @brief 上报节点状态信息
 * @return 上报是否成功
 * @note 节点状态信息结构如：
 * {
 *     "node_type":"ACCESS",
 *     "node_ip":"192.168.11.12",
 *     "node_port":9988,
 *     "access_ip":"120.234.2.106",
 *     "access_port":10001,
 *     "worker_num":10,
 *     "active_time":16879561651.06,
 *     "node":{
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":495870
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}}
 *     ]
 * }
 */
bool Manager::ReportToBeacon()
{
    if (std::string("BEACON") == m_stManagerInfo.strNodeType)
    {
        return(true);
    }
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgBody oMsgBody;
    CJsonObject oReportData;
    CJsonObject oMember;
    oReportData.Add("node_type", m_stManagerInfo.strNodeType);
    oReportData.Add("node_id", m_stManagerInfo.uiNodeId);
    oReportData.Add("node_ip", m_stManagerInfo.strHostForServer);
    oReportData.Add("node_port", m_stManagerInfo.iPortForServer);
    if (m_stManagerInfo.strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_stManagerInfo.strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_stManagerInfo.strHostForClient);
    }
    if (m_stManagerInfo.iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_stManagerInfo.iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_stManagerInfo.iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    auto worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second.iLoad;
        iConnect += worker_iter->second.iConnect;
        iRecvNum += worker_iter->second.iRecvNum;
        iRecvByte += worker_iter->second.iRecvByte;
        iSendNum += worker_iter->second.iSendNum;
        iSendByte += worker_iter->second.iSendByte;
        iClientNum += worker_iter->second.iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second.iLoad);
        oMember.Add("connect", worker_iter->second.iConnect);
        oMember.Add("recv_num", worker_iter->second.iRecvNum);
        oMember.Add("recv_byte", worker_iter->second.iRecvByte);
        oMember.Add("send_num", worker_iter->second.iSendNum);
        oMember.Add("send_byte", worker_iter->second.iSendByte);
        oMember.Add("client", worker_iter->second.iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
    oMsgBody.set_data(oReportData.ToString());
    LOG4_TRACE("%s", oReportData.ToString().c_str());

    Broadcast("BEACON", CMD_REQ_NODE_STATUS_REPORT, GetSequence(), oMsgBody);
    return(true);
}

bool Manager::AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("%s", strIdentify.c_str());
    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        m_mapNamedSocketChannel.insert(std::make_pair(strIdentify, pChannel));
    }
    else
    {
        return(false);
    }
    pChannel->m_pImpl->SetIdentify(strIdentify);
    return(true);

}

void Manager::DelNamedSocketChannel(const std::string& strIdentify)
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

void Manager::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    m_pSessionNode->AddNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType && m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(true);
    }
}

void Manager::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s, %s", strNodeType.c_str(), strIdentify.c_str());
    if (std::string("BEACON") == strNodeType)
    {
        LOG4_WARNING("node_type BEACON should not be deleted!");
    }

    m_pSessionNode->DelNode(strNodeType, strIdentify);

    std::string strOnlineNode;
    if (std::string("LOGGER") == strNodeType && !m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        m_pLogger->EnableNetLogger(false);
    }
}

bool Manager::AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%d, %u", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    ev_timer* timer_watcher = pChannel->m_pImpl->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        timer_watcher = pChannel->m_pImpl->AddTimerWatcher();
        if (NULL == timer_watcher)
        {
            return(false);
        }
        pChannel->m_pImpl->SetKeepAlive(ev_now(m_loop));
        ev_timer_init (timer_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        return(true);
    }
    else
    {
        ev_timer_stop(m_loop, timer_watcher);
        ev_timer_set(timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, timer_watcher);
    }
    return(true);
}

bool Manager::AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout)
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
    pData->pLabor = this;
    pData->iAddr = iAddr;
    timeout_watcher->data = (void*)pData;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

std::shared_ptr<SocketChannel> Manager::CreateChannel(int iFd, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("iFd", iFd);
    auto iter = m_mapSocketChannel.find(iFd);
    if (iter == m_mapSocketChannel.end())
    {
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
        pChannel->m_pImpl->SetLabor(this);
        if (pChannel->m_pImpl->Init(eCodecType))
        {
            m_mapSocketChannel.insert(std::make_pair(iFd, pChannel));
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

bool Manager::DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel)
{
    LOG4_TRACE("fd[%d], seq[%u]", pChannel->m_pImpl->GetFd(), pChannel->m_pImpl->GetSequence());
    auto name_channel_iter = m_mapNamedSocketChannel.find(pChannel->m_pImpl->GetIdentify());
    if (name_channel_iter != m_mapNamedSocketChannel.end())
    {
        m_mapNamedSocketChannel.erase(name_channel_iter);
    }
    DelEvents(pChannel->m_pImpl->MutableIoWatcher());
    DelEvents(pChannel->m_pImpl->MutableTimerWatcher());
    auto iter = m_mapSocketChannel.find(pChannel->m_pImpl->GetFd());
    if (iter != m_mapSocketChannel.end())
    {
        m_mapSocketChannel.erase(iter);
    }
    pChannel->m_pImpl->Abort();
    return(true);
}

void Manager::SetWorkerLoad(int iPid, CJsonObject& oJsonLoad)
{
    auto iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        oJsonLoad.Get("load", iter->second.iLoad);
        oJsonLoad.Get("connect", iter->second.iConnect);
        oJsonLoad.Get("recv_num", iter->second.iRecvNum);
        oJsonLoad.Get("recv_byte", iter->second.iRecvByte);
        oJsonLoad.Get("send_num", iter->second.iSendNum);
        oJsonLoad.Get("send_byte", iter->second.iSendByte);
        oJsonLoad.Get("client", iter->second.iClientNum);
        iter->second.dBeatTime = ev_now(m_loop);
    }
}

void Manager::AddWorkerLoad(int iPid, int iLoad)
{
    LOG4_TRACE(" ");
    auto iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        iter->second.iLoad += iLoad;
    }
}

bool Manager::OnWorkerData(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("cmd %u, seq %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (CMD_REQ_UPDATE_WORKER_LOAD == oInMsgHead.cmd())    // 新请求
    {
        auto iter = m_mapWorkerFdPid.find(pChannel->m_pImpl->GetFd());
        if (iter != m_mapWorkerFdPid.end())
        {
            CJsonObject oJsonLoad;
            oJsonLoad.Parse(oInMsgBody.data());
            SetWorkerLoad(iter->second, oJsonLoad);
        }
    }
    else
    {
        LOG4_WARNING("unknow cmd %d from worker!", oInMsgHead.cmd());
    }
    return(true);
}

bool Manager::OnDataAndTransferFd(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("cmd %u, seq %u", oInMsgHead.cmd(), oInMsgHead.seq());
    MsgBody oOutMsgBody;
    LOG4_TRACE("oInMsgHead.cmd() = %u, seq() = %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (oInMsgHead.cmd() == CMD_REQ_CONNECT_TO_WORKER)
    {
        int iWorkerIndex = std::stoi(oInMsgBody.data());
        std::unordered_map<int, tagWorkerAttr>::iterator worker_iter;
        for (worker_iter = m_mapWorker.begin();
                        worker_iter != m_mapWorker.end(); ++worker_iter)
        {
            if (iWorkerIndex == worker_iter->second.iWorkerIndex)
            {
                int iErrno = SocketChannel::SendChannelFd(
                    worker_iter->second.iDataFd, pChannel->m_pImpl->GetFd(), (int)pChannel->m_pImpl->GetCodecType(), m_pLogger);
                if (iErrno != 0)
                {
                    DiscardSocketChannel(pChannel);
                    return(false);
                }
                DiscardSocketChannel(pChannel);
                return(true);
            }
        }
        if (worker_iter == m_mapWorker.end())
        {
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_NO_SUCH_WORKER_INDEX);
            oOutMsgBody.mutable_rsp_result()->set_msg("no such worker index!");
        }
    }
    else
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
        oOutMsgBody.mutable_rsp_result()->set_msg("unknow cmd!");
        LOG4_DEBUG("unknow cmd %d!", oInMsgHead.cmd());
    }

    E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
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
        return(false);
    }
    return(true);
}

bool Manager::OnBeaconData(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("cmd %u, seq %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求，直接转发给Worker，并回复beacon已收到请求
    {
        if (CMD_REQ_BEAT == oInMsgHead.cmd())   // beacon发过来的心跳包
        {
            MsgBody oOutMsgBody = oInMsgBody;
            E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
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
                return(false);
            }
            return(true);
        }
        else if (CMD_REQ_TELL_WORKER == oInMsgHead.cmd()) 
        {
            MsgBody oOutMsgBody;
            TargetWorker oInTargetWorker;
            TargetWorker oOutTargetWorker;
            if (oInTargetWorker.ParseFromString(oInMsgBody.data()))
            {
                LOG4_DEBUG("AddNodeIdentify(%s, %s)!", oInTargetWorker.node_type().c_str(),
                                oInTargetWorker.worker_identify().c_str());
                AddNamedSocketChannel(oInTargetWorker.worker_identify(), pChannel);
                AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
                oOutTargetWorker.set_worker_identify(GetNodeIdentify());
                oOutTargetWorker.set_node_type(m_stManagerInfo.strNodeType);
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
                oOutMsgBody.mutable_rsp_result()->set_msg("OK");
            }
            else
            {
                oOutTargetWorker.set_worker_identify("unknow");
                oOutTargetWorker.set_node_type(m_stManagerInfo.strNodeType);
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
                oOutMsgBody.mutable_rsp_result()->set_msg("WorkerLoad ParseFromString error!");
                LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
            }
            oOutMsgBody.set_data(oOutTargetWorker.SerializeAsString());

            E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(CMD_RSP_TELL_WORKER, oInMsgHead.seq(), oOutMsgBody);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                eCodecStatus = pChannel->m_pImpl->Send();
            }
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
                return(false);
            }
            return(true);
        }
        SendToWorker(oInMsgHead.cmd(), oInMsgHead.seq(), oInMsgBody);
        MsgBody oOutMsgBody;
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
        oOutMsgBody.mutable_rsp_result()->set_msg("OK");
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
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
            return(false);
        }
        return(true);
    }
    else    // 回调
    {
        if (CMD_RSP_NODE_REGISTER == oInMsgHead.cmd()) //Manager这层只有向beacon注册会收到回调，上报状态不收回调或者收到回调不必处理
        {
            if (ERR_OK == oInMsgBody.rsp_result().code())
            {
                CJsonObject oNode(oInMsgBody.data());
                oNode.Get("node_id", m_stManagerInfo.uiNodeId);
                SendToWorker(CMD_REQ_REFRESH_NODE_ID, oInMsgHead.seq(), oInMsgBody);
                RemoveIoWriteEvent(pChannel);
            }
            else
            {
                LOG4_ERROR("register to beacon error %d: %s!", oInMsgBody.rsp_result().code(), oInMsgBody.rsp_result().msg().c_str());
            }
        }
        else if (CMD_RSP_CONNECT_TO_WORKER == oInMsgHead.cmd()) // 连接beacon时的回调  TODO delete
        {
            MsgBody oOutMsgBody;
            TargetWorker oTargetWorker;
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d", m_stManagerInfo.strHostForServer.c_str(), m_stManagerInfo.iPortForServer);
            oTargetWorker.set_worker_identify(szWorkerIdentify);
            oTargetWorker.set_node_type(m_stManagerInfo.strNodeType);
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
            oOutMsgBody.mutable_rsp_result()->set_msg("OK");
            oOutMsgBody.set_data(oTargetWorker.SerializeAsString());
            E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(CMD_REQ_TELL_WORKER, GetSequence(), oOutMsgBody);
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
                return(false);
            }
            return(true);
        }
        else if (CMD_RSP_TELL_WORKER == oInMsgHead.cmd()) // 连接beacon时的回调
        {
            pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
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
                return(false);
            }
            return(true);
        }
    }
    return(true);
}

bool Manager::OnNodeNotify(const MsgBody& oMsgBody)
{
    CJsonObject oJson;
    if (!oJson.Parse(oMsgBody.data()))
    {
        LOG4_ERROR("failed to parse msgbody content!");
        return(false);
    }
    std::string strNodeType;
    std::string strNodeHost;
    int iNodePort = 0;
    int iWorkerNum = 0;
    for (int i = 0; i < oJson["add_nodes"].GetArraySize(); ++i)
    {
        if (oJson["add_nodes"][i].Get("node_type",strNodeType)
                && oJson["add_nodes"][i].Get("node_ip",strNodeHost)
                && oJson["add_nodes"][i].Get("node_port",iNodePort)
                && oJson["add_nodes"][i].Get("worker_num",iWorkerNum))
        {
            if (std::string("LOGGER") == strNodeType)
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    std::shared_ptr<SocketChannel> pChannel = std::make_shared<SocketChannel>(m_pLogger, 0, 0);
                    std::ostringstream oss;
                    oss << strNodeHost << ":" << iNodePort << "." << j;
                    std::string strIdentify = std::move(oss.str());
                    AddNodeIdentify(strNodeType, strIdentify);
                    LOG4_DEBUG("AddNodeIdentify(%s,%s)", strNodeType.c_str(), strIdentify.c_str());
                }
            }
        }
    }

    for (int i = 0; i < oJson["del_nodes"].GetArraySize(); ++i)
    {
        if (oJson["del_nodes"][i].Get("node_type",strNodeType)
                && oJson["del_nodes"][i].Get("node_ip",strNodeHost)
                && oJson["del_nodes"][i].Get("node_port",iNodePort)
                && oJson["del_nodes"][i].Get("worker_num",iWorkerNum))
        {
            if (std::string("LOGGER") == strNodeType)
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    std::ostringstream oss;
                    oss << strNodeHost << ":" << iNodePort << "." << j;
                    std::string strIdentify = std::move(oss.str());
                    if (std::string("LOGGER") == strNodeType)
                    {
                        DelNodeIdentify(strNodeType, strIdentify);
                        LOG4_DEBUG("DelNodeIdentify(%s,%s)", strNodeType.c_str(), strIdentify.c_str());
                    }
                }
            }
        }
    }
    return(true);
}

} /* namespace neb */
