/*******************************************************************************
 * Project:  Nebula
 * @file     Manager.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include "util/proctitle_helper.h"
#include "util/process_helper.h"
#include "Manager.hpp"

namespace neb
{

void Manager::SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        if (SIGCHLD == watcher->signum)
        {
            pManager->ChildTerminated(watcher);
        }
        else
        {
            pManager->ManagerTerminated(watcher);
        }
    }
}

void Manager::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)watcher->data;
        pManager->CheckWorker();
        pManager->ReportToKeeper();
    }
}

void Manager::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Channel* pChannel = (Channel*)watcher->data;
        Manager* pManager = (Manager*)pChannel->m_pLabor;
        if (revents & EV_READ)
        {
            pManager->IoRead(pChannel);
        }
        if (revents & EV_WRITE)
        {
            pManager->IoWrite(pChannel);
        }
        if (revents & EV_ERROR)
        {
            pManager->IoError(pChannel);
        }
    }
}

void Manager::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Channel* pChannel = (Channel*)watcher->data;
        Manager* pManager = (Manager*)pChannel->m_pLabor;
        pManager->IoTimeout(pChannel);
    }
}

void Manager::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Manager* pManager = (Manager*)(watcher->data);
#ifndef NODE_TYPE_BEACON
        pManager->ReportToKeeper();
#endif
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
        pManager->ClientConnFrequencyTimeout(pData, watcher);
    }
}

Manager::Manager(const std::string& strConfFile)
    : m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480), m_strConfFile(strConfFile),
      m_uiNodeId(0), m_iPortForServer(9988), m_iPortForClient(0), m_iGatewayPort(0), m_uiWorkerNum(10),
      m_eCodec(CODEC_PROTOBUF), m_dAddrStatInterval(60.0), m_iAddrPermitNum(10),
      m_iLogLevel(log4cplus::INFO_LOG_LEVEL), m_iWorkerBeat(11), m_iRefreshCalc(0),
      m_iS2SListenFd(-1), m_iC2SListenFd(-1), m_loop(NULL), m_pPeriodicTaskWatcher(NULL)//, m_pCmdConnect(NULL)
{
    if (strConfFile == "")
    {
        std::cerr << "error: no config file!" << std::endl;
        exit(1);
    }

    if (!GetConf())
    {
        std::cerr << "GetConf() error!" << std::endl;
        exit(-1);
    }
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    daemonize(m_oCurrentConf("server_name").c_str());
    Init();
    m_iWorkerBeat = (gc_iBeatInterval * 2) + 1;
    CreateEvents();
    CreateWorker();
    RegisterToKeeper();
}

Manager::~Manager()
{
    Destroy();
}

bool Manager::ManagerTerminated(struct ev_signal* watcher)
{
    LOG4_WARN("%s terminated by signal %d!", m_oCurrentConf("server_name").c_str(), watcher->signum);
    ev_break (m_loop, EVBREAK_ALL);
    exit(-1);
}

bool Manager::ChildTerminated(struct ev_signal* watcher)
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

bool Manager::IoRead(Channel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pChannel->GetFd() == m_iS2SListenFd)
    {
#ifdef UNIT_TEST
        return(FdTransfer(pChannel->GetFd()));
#endif
        return(AcceptServerConn(pChannel->GetFd()));
    }
#ifdef NODE_TYPE_ACCESS
    else if (pChannel->GetFd() == m_iC2SListenFd)
    {
        return(FdTransfer(pChannel->GetFd()));
    }
#endif
    else
    {
        return(DataRecvAndHandle(pChannel));
    }
}

bool Manager::FdTransfer(int iFd)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
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
    int iKeepAlive = 1;
    int iKeepIdle = 1000;
    int iKeepInterval = 10;
    int iKeepCount = 3;
    int iTcpNoDelay = 1;
    if (setsockopt(iAcceptFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0)
    {
        LOG4_WARN("fail to set SO_KEEPALIVE");
    }
    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
    {
        LOG4_WARN("fail to set TCP_KEEPIDLE");
    }
    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
    {
        LOG4_WARN("fail to set TCP_KEEPINTVL");
    }
    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
    {
        LOG4_WARN("fail to set TCP_KEEPCNT");
    }
    if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
    {
        LOG4_WARN("fail to set TCP_NODELAY");
    }

    std::map<in_addr_t, uint32>::iterator iter = m_mapClientConnFrequency.find(stClientAddr.sin_addr.s_addr);
    if (iter == m_mapClientConnFrequency.end())
    {
        m_mapClientConnFrequency.insert(std::pair<in_addr_t, uint32>(stClientAddr.sin_addr.s_addr, 1));
        AddClientConnFrequencyTimeout(stClientAddr.sin_addr.s_addr, m_dAddrStatInterval);
    }
    else
    {
        iter->second++;
#ifdef NODE_TYPE_ACCESS
        if (iter->second > (uint32)m_iAddrPermitNum)
        {
            LOG4_WARN("client addr %d had been connected more than %u times in %f seconds, it's not permitted",
                            stClientAddr.sin_addr.s_addr, m_iAddrPermitNum, m_dAddrStatInterval);
            return(false);
        }
#endif
    }

    std::pair<int, int> worker_pid_fd = GetMinLoadWorkerDataFd();
    if (worker_pid_fd.second > 0)
    {
        LOG4_DEBUG("send new fd %d to worker communication fd %d",
                        iAcceptFd, worker_pid_fd.second);
        int iCodec = m_eCodec;
        //int iErrno = send_fd(worker_pid_fd.second, iAcceptFd);
        int iErrno = send_fd_with_attr(worker_pid_fd.second, iAcceptFd, szIpAddr, 16, iCodec);
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
    //LOG4_TRACE("%s()", __FUNCTION__);
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
            LOG4_WARN("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            LOG4_WARN("fail to set SO_KEEPIDLE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            LOG4_WARN("fail to set SO_KEEPINTVL");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&iKeepCount, sizeof (iKeepCount)) < 0)
        {
            LOG4_WARN("fail to set SO_KEEPALIVE");
        }
        if (setsockopt(iAcceptFd, IPPROTO_TCP, TCP_NODELAY, (void*)&iTcpNoDelay, sizeof(iTcpNoDelay)) < 0)
        {
            LOG4_WARN("fail to set TCP_NODELAY");
        }
        uint32 ulSeq = GetSequence();
        x_sock_set_block(iAcceptFd, 0);
        Channel* pChannel = CreateChannel(iAcceptFd, CODEC_PROTOBUF);
        if (NULL != pChannel)
        {
            AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            AddIoReadEvent(pChannel);
        }
    }
    return(false);
}

bool Manager::DataRecvAndHandle(Channel* pChannel)
{
    LOG4_DEBUG("fd %d, seq %llu", pChannel->GetFd(), pChannel->GetSequence());
    E_CODEC_STATUS eCodecStatus;
    if (CODEC_HTTP == pChannel->GetCodecType())   // Manager只有pb协议编解码
    {
//        HttpMsg oHttpMsg;
//        eCodecStatus = pChannel->Recv(oHttpMsg);
        DestroyConnect(pChannel);
        return(false);
    }
    else
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        eCodecStatus = pChannel->Recv(oMsgHead, oMsgBody);
        while (CODEC_STATUS_OK == eCodecStatus)
        {
            std::map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(pChannel->GetFd());
            if (worker_fd_iter == m_mapWorkerFdPid.end())   // 其他Server发过来要将连接传送到某个指定Worker进程信息
            {
                std::map<std::string, tagChannelContext>::iterator center_iter = m_mapKeeperCtx.find(pChannel->GetIdentify());
                if (center_iter == m_mapKeeperCtx.end())       // 不是与keeper连接
                {
                    OnDataAndTransferFd(pChannel, oMsgHead, oMsgBody);
                }
                else    // 与keeper连接
                {
                    OnBeaconData(pChannel, oMsgHead, oMsgBody);
                }
            }
            else    // Worker进程发过来的消息
            {
                OnWorkerData(pChannel, oMsgHead, oMsgBody);
            }
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->Fetch(oMsgHead, oMsgBody);
        }

        if (CODEC_STATUS_PAUSE == eCodecStatus)
        {
            return(true);
        }
        else    // 编解码出错或连接关闭或连接中断
        {
            DestroyConnect(pChannel);
            return(false);
        }
    }
}

bool Manager::IoWrite(Channel* pChannel)
{
    LOG4_TRACE("%s(%d)", __FUNCTION__, pChannel->GetFd());
    if (CHANNEL_STATUS_INIT == pChannel->GetChannelStatus())  // connect之后的第一个写事件
    {
        std::map<uint32, int>::iterator index_iter = m_mapSeq2WorkerIndex.find(pChannel->GetSequence());
        if (index_iter != m_mapSeq2WorkerIndex.end())
        {
            tagChannelContext stCtx;
            stCtx.iFd = pChannel->GetFd();
            stCtx.ulSeq = pChannel->GetSequence();
            //AddInnerFd(stCtx); 只有Worker需要
            std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.find(pChannel->GetIdentify());
            if (keeper_iter == m_mapKeeperCtx.end())
            {
                m_mapKeeperCtx.insert(std::pair<std::string, tagChannelContext>(pChannel->GetIdentify(), stCtx));
            }
            else
            {
                keeper_iter->second = stCtx;
            }
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            ConnectWorker oConnWorker;
            oConnWorker.set_worker_index(index_iter->second);
            oMsgBody.set_data(oConnWorker.SerializeAsString());
            oMsgHead.set_cmd(CMD_REQ_CONNECT_TO_WORKER);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_len(oMsgBody.ByteSize());
            m_mapSeq2WorkerIndex.erase(index_iter);
            LOG4_DEBUG("send after connect");
            SendTo(stCtx, oMsgHead, oMsgBody);
            return(true);
        }
        return(false);
    }
    else
    {
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
            DestroyConnect(pChannel);
        }
    }
    return(true);
}

bool Manager::IoError(Channel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    return(false);
}

bool Manager::IoTimeout(Channel* pChannel)
{
    ev_tstamp after = pChannel->GetActiveTime() - ev_now(m_loop) + m_dIoTimeout;
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer* watcher = pChannel->MutableTimerWatcher();
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // IO已超时，关闭文件描述符并清理相关资源
    {
        LOG4_DEBUG("%s()", __FUNCTION__);
        DestroyConnect(pChannel);
        return(false);
    }
}

bool Manager::ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    bool bRes = false;
    std::map<in_addr_t, uint32>::iterator iter = m_mapClientConnFrequency.find(pData->iAddr);
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
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

bool Manager::InitLogger(const CJsonObject& oJsonConf)
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
        std::string strLogname = oJsonConf("log_path") + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_strHostForServer << ":" << m_iPortForServer;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_level", iLogLevel);
        log4cplus::initialize();
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        log4cplus::SharedAppenderPtr file_append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        file_append->setName(strLogname);
        file_append->setLayout(layout);
        //log4cplus::Logger::getRoot().addAppender(file_append);
        m_oLogger = log4cplus::Logger::getInstance(strLogname);
        m_oLogger.setLogLevel(iLogLevel);
        m_oLogger.addAppender(file_append);
        if (oJsonConf.Get("socket_logging_host", strLoggingHost) && oJsonConf.Get("socket_logging_port", iLoggingPort))
        {
            log4cplus::SharedAppenderPtr socket_append(new log4cplus::SocketAppender(
                            strLoggingHost, iLoggingPort, ssServerName.str()));
            socket_append->setName(ssServerName.str());
            socket_append->setLayout(layout);
            socket_append->setThreshold(log4cplus::INFO_LOG_LEVEL);
            m_oLogger.addAppender(socket_append);
        }
        LOG4_INFO("%s program begin, and work path %s...", oJsonConf("server_name").c_str(), m_strWorkPath.c_str());
        m_bInitLogger = true;
        return(true);
    }
}

bool Manager::SetProcessName(const CJsonObject& oJsonConf)
{
    ngx_setproctitle(oJsonConf("server_name").c_str());
    return(true);
}

void Manager::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool Manager::SendTo(const tagChannelContext& stCtx)
{
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            E_CODEC_STATUS eCodecStatus = iter->second->Send();
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                RemoveIoWriteEvent(iter->second);
            }
            else if (CODEC_STATUS_PAUSE == eCodecStatus)
            {
                AddIoWriteEvent(iter->second);
            }
            else
            {
                DestroyConnect(iter->second);
            }
        }
    }
    return(false);
}

bool Manager::SendTo(const tagChannelContext& stCtx, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(cmd[%u], seq[%u])", __FUNCTION__, oMsgHead.cmd(), oMsgHead.seq());
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            E_CODEC_STATUS eCodecStatus = iter->second->Send(oMsgHead, oMsgBody);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                RemoveIoWriteEvent(iter->second);
            }
            else if (CODEC_STATUS_PAUSE == eCodecStatus)
            {
                AddIoWriteEvent(iter->second);
            }
            else
            {
                DestroyConnect(iter->second);
            }
            return(true);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %llu not match the sequence %llu in m_mapChannel",
                            stCtx.iFd, stCtx.ulSeq, iter->second->GetSequence());
            return(false);
        }
    }
}

bool Manager::SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            iter->second->SetIdentify(strIdentify);
            return(true);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                    stCtx.iFd, stCtx.ulSeq, iter->second->GetSequence());
            return(false);
        }
    }
}

bool Manager::AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
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

    std::map<int, int>::iterator worker_fd_iter = m_mapWorkerFdPid.find(iFd);
    if (worker_fd_iter != m_mapWorkerFdPid.end())
    {
        LOG4_TRACE("iFd = %d found in m_mapWorkerFdPid", iFd);
    }

    x_sock_set_block(iFd, 0);
    int reuse = 1;
    ::setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    Channel* pChannel = CreateChannel(iFd, CODEC_PROTOBUF);
    if (NULL != pChannel)
    {
        AddIoTimeout(pChannel, 1.5);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->SetIdentify(strIdentify);
        pChannel->Send(oMsgHead, oMsgBody);
        m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(pChannel->GetSequence(), iWorkerIndex));
        std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.find(strIdentify);
        if (keeper_iter != m_mapKeeperCtx.end())
        {
            keeper_iter->second.iFd = iFd;
            keeper_iter->second.ulSeq = pChannel->GetSequence();
        }
        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        return(true);
    }
    close(iFd);
    return(false);
}

bool Manager::GetConf()
{
    char szFilePath[256] = {0};
    //char szFileName[256] = {0};
    if (m_strWorkPath.length() == 0)
    {
        if (getcwd(szFilePath, sizeof(szFilePath)))
        {
            m_strWorkPath = szFilePath;
            //std::cout << "work dir: " << m_strWorkPath << std::endl;
        }
        else
        {
            return(false);
        }
    }
    m_oLastConf = m_oCurrentConf;
    //snprintf(szFileName, sizeof(szFileName), "%s/%s", m_strWorkPath.c_str(), m_strConfFile.c_str());
    std::ifstream fin(m_strConfFile.c_str());
    if (fin.good())
    {
        std::stringstream ssContent;
        ssContent << fin.rdbuf();
        if (!m_oCurrentConf.Parse(ssContent.str()))
        {
            ssContent.str("");
            fin.close();
            m_oCurrentConf = m_oLastConf;
            return(false);
        }
        ssContent.str("");
        fin.close();
    }
    else
    {
        return(false);
    }

    if (m_oLastConf.ToString() != m_oCurrentConf.ToString())
    {
        m_oCurrentConf.Get("io_timeout", m_dIoTimeout);
        if (m_oLastConf.ToString().length() == 0)
        {
            m_uiWorkerNum = strtoul(m_oCurrentConf("process_num").c_str(), NULL, 10);
            m_oCurrentConf.Get("node_type", m_strNodeType);
            m_oCurrentConf.Get("inner_host", m_strHostForServer);
            m_oCurrentConf.Get("inner_port", m_iPortForServer);
            m_oCurrentConf.Get("access_host", m_strHostForClient);
            m_oCurrentConf.Get("access_port", m_iPortForClient);
            m_oCurrentConf.Get("gateway", m_strGateway);
            m_oCurrentConf.Get("gateway_port", m_iGatewayPort);
        }
        int32 iCodec;
        if (m_oCurrentConf.Get("access_codec", iCodec))
        {
            m_eCodec = E_CODEC_TYPE(iCodec);
        }
        m_oCurrentConf["permission"]["addr_permit"].Get("stat_interval", m_dAddrStatInterval);
        m_oCurrentConf["permission"]["addr_permit"].Get("permit_num", m_iAddrPermitNum);
        if (m_oCurrentConf.Get("log_level", m_iLogLevel))
        {
            switch (m_iLogLevel)
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
                    m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            m_iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
    }
    return(true);
}

bool Manager::Init()
{
    InitLogger(m_oCurrentConf);

    socklen_t addressLen = 0;
    int queueLen = 100;
    int reuse = 1;
    int timeout = 1;

#ifdef NODE_TYPE_ACCESS
    // 接入节点才需要监听客户端连接
    struct sockaddr_in stAddrOuter;
    struct sockaddr *pAddrOuter;
    stAddrOuter.sin_family = AF_INET;
    stAddrOuter.sin_port = htons(m_iPortForClient);
    stAddrOuter.sin_addr.s_addr = inet_addr(m_strHostForClient.c_str());
    pAddrOuter = (struct sockaddr*)&stAddrOuter;
    addressLen = sizeof(struct sockaddr);
    m_iC2SListenFd = socket(pAddrOuter->sa_family, SOCK_STREAM, 0);
    if (m_iC2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    reuse = 1;
    timeout = 1;
    ::setsockopt(m_iC2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ::setsockopt(m_iC2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
    if (bind(m_iC2SListenFd, pAddrOuter, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_iC2SListenFd);
        m_iC2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_iC2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_iC2SListenFd);
        m_iC2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
#endif

    struct sockaddr_in stAddrInner;
    struct sockaddr *pAddrInner;
    stAddrInner.sin_family = AF_INET;
    stAddrInner.sin_port = htons(m_iPortForServer);
    stAddrInner.sin_addr.s_addr = inet_addr(m_strHostForServer.c_str());
    pAddrInner = (struct sockaddr*)&stAddrInner;
    addressLen = sizeof(struct sockaddr);
    m_iS2SListenFd = socket(pAddrInner->sa_family, SOCK_STREAM, 0);
    if (m_iS2SListenFd < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        int iErrno = errno;
        exit(iErrno);
    }
    reuse = 1;
    timeout = 1;
    ::setsockopt(m_iS2SListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ::setsockopt(m_iS2SListenFd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(int));
    if (bind(m_iS2SListenFd, pAddrInner, addressLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }
    if (listen(m_iS2SListenFd, queueLen) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        close(m_iS2SListenFd);
        m_iS2SListenFd = -1;
        int iErrno = errno;
        exit(iErrno);
    }

    // 创建到beacon的连接信息
    for (int i = 0; i < m_oCurrentConf["beacon"].GetArraySize(); ++i)
    {
        std::string strIdentify = m_oCurrentConf["beacon"][i]("host") + std::string(":")
            + m_oCurrentConf["beacon"][i]("port") + std::string(".0");     // KeeperServer只有一个Worker
        tagChannelContext stCtx;
        LOG4_TRACE("m_mapKeeperMsgShell.insert(%s, fd %d, seq %llu) = %u",
                        strIdentify.c_str(), stCtx.iFd, stCtx.ulSeq);
        m_mapKeeperCtx.insert(std::pair<std::string, tagChannelContext>(strIdentify, stCtx));
    }

    return(true);
}

void Manager::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapCmd.begin();
                    cmd_iter != m_mapCmd.end(); ++cmd_iter)
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
    }
    m_mapCmd.clear();

    m_mapWorker.clear();
    m_mapWorkerFdPid.clear();
    m_mapWorkerRestartNum.clear();
    for (std::map<int, Channel*>::iterator iter = m_mapChannel.begin();
                    iter != m_mapChannel.end(); ++iter)
    {
        DestroyConnect(iter);
    }
    m_mapChannel.clear();
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

void Manager::CreateWorker()
{
    LOG4_TRACE("%s", __FUNCTION__);
    int iPid = 0;

    for (unsigned int i = 0; i < m_uiWorkerNum; ++i)
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
            close(m_iS2SListenFd);
#ifdef NODE_TYPE_ACCESS
            close(m_iC2SListenFd);
#endif
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker worker(m_strWorkPath, iControlFds[1], iDataFds[1], i, m_oCurrentConf);
            worker.Run();
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
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iPid));
            Channel* pChannelData = CreateChannel(iControlFds[0], CODEC_PROTOBUF);
            Channel* pChannelControl = CreateChannel(iDataFds[0], CODEC_PROTOBUF);
            AddIoReadEvent(pChannelData);
            AddIoReadEvent(pChannelControl);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_szErrBuff, 1024));
        }
    }
}

bool Manager::CreateEvents()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    m_loop = ev_loop_new(EVFLAG_FORKCHECK | EVFLAG_SIGNALFD);
    if (m_loop == NULL)
    {
        return(false);
    }
    Channel* pChannelListen = CreateChannel(m_iS2SListenFd, CODEC_PROTOBUF);
    AddIoReadEvent(pChannelListen);
#ifdef NODE_TYPE_ACCESS
    pChannelListen = CreateChannel(m_iC2SListenFd, m_eCodec);
    AddIoReadEvent(pChannelListen);
#endif

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

bool Manager::RegisterToKeeper()
{
    if (m_mapKeeperCtx.size() == 0)
    {
        return(true);
    }
    LOG4_DEBUG("%s()", __FUNCTION__);
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    CJsonObject oReportData;
    CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    if (m_strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_strHostForClient);
    }
    if (m_iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    std::map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
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
    oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_len(oMsgBody.ByteSize());
    std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.begin();
    for (; keeper_iter != m_mapKeeperCtx.end(); ++keeper_iter)
    {
        if (keeper_iter->second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(keeper_iter->first, oMsgHead, oMsgBody);
        }
        else
        {
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(keeper_iter->second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool Manager::RestartWorker(int iDeathPid)
{
    LOG4_DEBUG("%s(%d)", __FUNCTION__, iDeathPid);
    int iNewPid = 0;
    char errMsg[1024] = {0};
    std::map<int, tagWorkerAttr>::iterator worker_iter;
    std::map<int, int>::iterator fd_iter;
    std::map<int, int>::iterator restart_num_iter;
    worker_iter = m_mapWorker.find(iDeathPid);
    if (worker_iter != m_mapWorker.end())
    {
        LOG4_TRACE("restart worker %d, close control fd %d and data fd %d first.",
                        worker_iter->second.iWorkerIndex, worker_iter->second.iControlFd, worker_iter->second.iDataFd);
        int iWorkerIndex = worker_iter->second.iWorkerIndex;
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iControlFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second.iDataFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        DestroyConnect(m_mapChannel.find(worker_iter->second.iControlFd));
        DestroyConnect(m_mapChannel.find(worker_iter->second.iDataFd));
        m_mapWorker.erase(worker_iter);

        restart_num_iter = m_mapWorkerRestartNum.find(iWorkerIndex);
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
            close(m_iS2SListenFd);
#ifdef NODE_TYPE_ACCESS
            close(m_iC2SListenFd);
#endif
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker worker(m_strWorkPath, iControlFds[1], iDataFds[1], iWorkerIndex, m_oCurrentConf);
            worker.Run();
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
            LOG4_TRACE("m_mapWorker insert (iNewPid %d, worker_index %d)", iNewPid, iWorkerIndex);
            m_mapWorker.insert(std::pair<int, tagWorkerAttr>(iNewPid, stWorkerAttr));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFds[0], iNewPid));
            m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFds[0], iNewPid));
            Channel* pChannelData = CreateChannel(iControlFds[0], CODEC_PROTOBUF);
            Channel* pChannelControl = CreateChannel(iDataFds[0], CODEC_PROTOBUF);
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
            RegisterToKeeper();     // 重启Worker进程后向Keeper重发注册请求，以获得center下发其他节点的信息
            return(true);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, errMsg, 1024));
        }
    }
    return(false);
}

bool Manager::AddPeriodicTaskEvent()
{
    LOG4_DEBUG("%s()", __FUNCTION__);
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

bool Manager::AddIoReadEvent(Channel* pChannel)
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

bool Manager::AddIoWriteEvent(Channel* pChannel)
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

bool Manager::RemoveIoWriteEvent(Channel* pChannel)
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

bool Manager::DelEvents(ev_io* io_watcher)
{
    if (NULL == io_watcher)
    {
        return(false);
    }
    LOG4_TRACE("%s(fd %d)", __FUNCTION__, io_watcher->fd);
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

bool Manager::AddIoTimeout(Channel* pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_timer* timer_watcher = pChannel->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        timer_watcher = pChannel->AddTimerWatcher();
        if (NULL == timer_watcher)
        {
            return(false);
        }
        pChannel->SetKeepAlive(ev_now(m_loop));
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
    LOG4_TRACE("%s()", __FUNCTION__);
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

Channel* Manager::CreateChannel(int iFd, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(iFd %d)", __FUNCTION__, iFd);
    std::map<int, Channel*>::iterator iter;
    iter = m_mapChannel.find(iFd);
    if (iter == m_mapChannel.end())
    {
        Channel* pChannel = NULL;
        try
        {
            pChannel = new Channel(iFd, GetSequence());
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new channel for fd %d error: %s", e.what());
            return(NULL);
        }
        pChannel->SetLabor(this);
        pChannel->SetLogger(&m_oLogger);
        if (pChannel->Init(eCodecType))
        {
            m_mapChannel.insert(std::pair<int, Channel*>(iFd, pChannel));
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

bool Manager::DestroyConnect(std::map<int, Channel*>::iterator iter)
{
    if (iter == m_mapChannel.end())
    {
        return(false);
    }
    std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.find(iter->second->GetIdentify());
    if (keeper_iter != m_mapKeeperCtx.end())
    {
        keeper_iter->second.iFd = 0;
        keeper_iter->second.ulSeq = 0;
    }
    DelEvents(iter->second->MutableIoWatcher());
    DelEvents(iter->second->MutableTimerWatcher());
    DELETE(iter->second);
    m_mapChannel.erase(iter);
    return(true);
}

bool Manager::DestroyConnect(Channel* pChannel)
{
    std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.find(pChannel->GetIdentify());
    if (keeper_iter != m_mapKeeperCtx.end())
    {
        keeper_iter->second.iFd = 0;
        keeper_iter->second.ulSeq = 0;
    }
    DelEvents(pChannel->MutableIoWatcher());
    DelEvents(pChannel->MutableTimerWatcher());
    DELETE(pChannel);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(pChannel->GetFd());
    if (iter != m_mapChannel.end())
    {
        m_mapChannel.erase(iter);
    }
    return(true);
}

std::pair<int, int> Manager::GetMinLoadWorkerDataFd()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_fd;
    std::map<int, tagWorkerAttr>::iterator iter;
    for (iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
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

void Manager::SetWorkerLoad(int iPid, CJsonObject& oJsonLoad)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
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
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagWorkerAttr>::iterator iter;
    iter = m_mapWorker.find(iPid);
    if (iter != m_mapWorker.end())
    {
        iter->second.iLoad += iLoad;
    }
}

const std::map<int, tagWorkerAttr>& Manager::GetWorkerAttr() const
{
    return(m_mapWorker);
}

bool Manager::CheckWorker()
{
    LOG4_TRACE("%s()", __FUNCTION__);

    std::map<int, tagWorkerAttr>::iterator worker_iter;
    for (worker_iter = m_mapWorker.begin();
                    worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        LOG4_TRACE("now %lf, worker_beat %lf, worker_beat %d",
                        ev_now(m_loop), worker_iter->second.dBeatTime, m_iWorkerBeat);
        if ((ev_now(m_loop) - worker_iter->second.dBeatTime) > m_iWorkerBeat)
        {
            LOG4CPLUS_INFO_FMT(m_oLogger, "worker_%d pid %d is unresponsive, "
                            "terminate it.", worker_iter->second.iWorkerIndex, worker_iter->first);
            kill(worker_iter->first, SIGKILL); //SIGINT);
//            RestartWorker(worker_iter->first);
        }
    }
    return(true);
}

void Manager::RefreshServer()
{
    LOG4_TRACE("%s(gc_iRefreshInterval %d, m_iLastRefreshCalc %d)", __FUNCTION__, gc_iRefreshInterval, m_iRefreshCalc);
    int iErrno = 0;
    ++m_iRefreshCalc;
    if (m_iRefreshCalc < gc_iRefreshInterval)
    {
        return;
    }
    m_iRefreshCalc = 0;
    if (GetConf())
    {
        if (m_oLastConf("log_level") != m_oCurrentConf("log_level"))
        {
            m_oLogger.setLogLevel(m_iLogLevel);
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            LogLevel oLogLevel;
            oLogLevel.set_log_level(m_iLogLevel);
            oMsgBody.set_data(oLogLevel.SerializeAsString());
            oMsgHead.set_cmd(CMD_REQ_SET_LOG_LEVEL);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
        }

        // 更新动态库配置或重新加载动态库
        if (m_oLastConf["so"].ToString() != m_oCurrentConf["so"].ToString())
        {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_data(m_oCurrentConf["so"].ToString());
            oMsgHead.set_cmd(CMD_REQ_RELOAD_SO);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
        }
        if (m_oLastConf["module"].ToString() != m_oCurrentConf["module"].ToString())
        {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            oMsgBody.set_data(m_oCurrentConf["module"].ToString());
            oMsgHead.set_cmd(CMD_REQ_RELOAD_MODULE);
            oMsgHead.set_seq(GetSequence());
            oMsgHead.set_len(oMsgBody.ByteSize());
            SendToWorker(oMsgHead, oMsgBody);
        }
    }
    else
    {
        LOG4_INFO("GetConf() error, please check the config file.", "");
    }
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
bool Manager::ReportToKeeper()
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    CJsonObject oReportData;
    CJsonObject oMember;
    oReportData.Add("node_type", m_strNodeType);
    oReportData.Add("node_id", m_uiNodeId);
    oReportData.Add("node_ip", m_strHostForServer);
    oReportData.Add("node_port", m_iPortForServer);
    if (m_strGateway.size() > 0)
    {
        oReportData.Add("access_ip", m_strGateway);
    }
    else
    {
        oReportData.Add("access_ip", m_strHostForClient);
    }
    if (m_iGatewayPort > 0)
    {
        oReportData.Add("access_port", m_iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", m_iPortForClient);
    }
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", ev_now(m_loop));
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    std::map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
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
    oMsgHead.set_cmd(CMD_REQ_NODE_STATUS_REPORT);
    oMsgHead.set_seq(GetSequence());
    oMsgHead.set_len(oMsgBody.ByteSize());
    LOG4_TRACE("%s()：  %s", __FUNCTION__, oReportData.ToString().c_str());

    std::map<std::string, tagChannelContext>::iterator keeper_iter = m_mapKeeperCtx.begin();
    for (; keeper_iter != m_mapKeeperCtx.end(); ++keeper_iter)
    {
        if (keeper_iter->second.iFd == 0)
        {
            oMsgHead.set_cmd(CMD_REQ_NODE_REGISTER);
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            AutoSend(keeper_iter->first, oMsgHead, oMsgBody);
        }
        else
        {
            LOG4_TRACE("%s() cmd %d", __FUNCTION__, oMsgHead.cmd());
            SendTo(keeper_iter->second, oMsgHead, oMsgBody);
        }
    }
    return(true);
}

bool Manager::SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    std::map<int, Channel*>::iterator worker_conn_iter;
    std::map<int, tagWorkerAttr>::iterator worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        worker_conn_iter = m_mapChannel.find(worker_iter->second.iControlFd);
        if (worker_conn_iter != m_mapChannel.end())
        {
            E_CODEC_STATUS eCodecStatus = worker_conn_iter->second->Send(oMsgHead, oMsgBody);
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
                DestroyConnect(worker_conn_iter->second);
            }
        }
    }
    return(true);
}

bool Manager::OnWorkerData(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    if (CMD_REQ_UPDATE_WORKER_LOAD == oInMsgHead.cmd())    // 新请求
    {
        std::map<int, int>::iterator iter = m_mapWorkerFdPid.find(pChannel->GetFd());
        if (iter != m_mapWorkerFdPid.end())
        {
            CJsonObject oJsonLoad;
            oJsonLoad.Parse(oInMsgBody.data());
            SetWorkerLoad(iter->second, oJsonLoad);
        }
    }
    else
    {
        LOG4_WARN("unknow cmd %d from worker!", oInMsgHead.cmd());
    }
    return(true);
}

bool Manager::OnDataAndTransferFd(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    int iErrno = 0;
    ConnectWorker oConnWorker;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    LOG4_TRACE("oInMsgHead.cmd() = %u, seq() = %u", oInMsgHead.cmd(), oInMsgHead.seq());
    if (oInMsgHead.cmd() == CMD_REQ_CONNECT_TO_WORKER)
    {
        if (oConnWorker.ParseFromString(oInMsgBody.data()))
        {
            std::map<int, tagWorkerAttr>::iterator worker_iter;
            for (worker_iter = m_mapWorker.begin();
                            worker_iter != m_mapWorker.end(); ++worker_iter)
            {
                if (oConnWorker.worker_index() == worker_iter->second.iWorkerIndex)
                {
                    // TODO 这里假设传送文件描述符都成功，是因为对传送文件描述符成功后Manager顺利回复消息，需要一个更好的解决办法
                    oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
                    oOutMsgBody.mutable_rsp_result()->set_msg("OK");
                    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
                    oOutMsgHead.set_seq(oInMsgHead.seq());
                    oOutMsgHead.set_len(oOutMsgBody.ByteSize());
                    E_CODEC_STATUS eCodecStatus = pChannel->Send(oOutMsgHead, oOutMsgBody);
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
                        DestroyConnect(pChannel);
                        return(false);
                    }

                    char szIp[16] = {0};
                    strncpy(szIp, "0.0.0.0", 16);   // 内网其他Server的IP不重要
                    int iErrno = send_fd_with_attr(worker_iter->second.iDataFd, pChannel->GetFd(), szIp, 16, CODEC_PROTOBUF);
                    //int iErrno = send_fd(worker_iter->second.iDataFd, stCtx.iFd);
                    if (iErrno != 0)
                    {
                        LOG4_ERROR("send_fd_with_attr error %d: %s!",
                                        iErrno, strerror_r(iErrno, m_szErrBuff, gc_iErrBuffLen));
                    }
                    DestroyConnect(pChannel);
                    return(false);
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
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
            oOutMsgBody.mutable_rsp_result()->set_msg("oConnWorker.ParseFromString() error!");
            LOG4_ERROR("oConnWorker.ParseFromString() error!");
        }
    }
    else
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
        oOutMsgBody.mutable_rsp_result()->set_msg("unknow cmd!");
        LOG4_DEBUG("unknow cmd %d!", oInMsgHead.cmd());
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_len(oOutMsgBody.ByteSize());
    }

    E_CODEC_STATUS eCodecStatus = pChannel->Send(oOutMsgHead, oOutMsgBody);
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
        DestroyConnect(pChannel);
        return(false);
    }
    return(true);
}

bool Manager::OnBeaconData(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %u)", __FUNCTION__, oInMsgHead.cmd(), oInMsgHead.seq());
    int iErrno = 0;
    if (gc_uiCmdReq & oInMsgHead.cmd())    // 新请求，直接转发给Worker，并回复beacon已收到请求
    {
        if (CMD_REQ_BEAT == oInMsgHead.cmd())   // beacon发过来的心跳包
        {
            MsgHead oOutMsgHead = oInMsgHead;
            MsgBody oOutMsgBody = oInMsgBody;
            oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
            E_CODEC_STATUS eCodecStatus = pChannel->Send(oOutMsgHead, oOutMsgBody);
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
                DestroyConnect(pChannel);
                return(false);
            }
            return(true);
        }
        SendToWorker(oInMsgHead, oInMsgBody);
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
        oOutMsgBody.mutable_rsp_result()->set_msg("OK");
        oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
        oOutMsgHead.set_seq(oInMsgHead.seq());
        oOutMsgHead.set_len(oOutMsgBody.ByteSize());
        E_CODEC_STATUS eCodecStatus = pChannel->Send(oOutMsgHead, oOutMsgBody);
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
            DestroyConnect(pChannel);
            return(false);
        }
        return(true);
    }
    else    // 回调
    {
        if (CMD_RSP_NODE_REGISTER == oInMsgHead.cmd()) //Manager这层只有向beacon注册会收到回调，上报状态不收回调或者收到回调不必处理
        {
            CJsonObject oNode(oInMsgBody.data());
            int iErrno = 0;
            oNode.Get("errcode", iErrno);
            if (0 == iErrno)
            {
                oNode.Get("node_id", m_uiNodeId);
                MsgHead oMsgHead;
                oMsgHead.set_cmd(CMD_REQ_REFRESH_NODE_ID);
                oMsgHead.set_seq(oInMsgHead.seq());
                oMsgHead.set_len(oInMsgHead.len());
                SendToWorker(oMsgHead, oInMsgBody);
                RemoveIoWriteEvent(pChannel);
            }
            else
            {
                LOG4_WARN("register to beacon error, errcode %d!", iErrno);
            }
        }
        else if (CMD_RSP_CONNECT_TO_WORKER == oInMsgHead.cmd()) // 连接beacon时的回调
        {
            MsgHead oOutMsgHead;
            MsgBody oOutMsgBody;
            TargetWorker oTargetWorker;
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d", m_strHostForServer.c_str(), m_iPortForServer);
            oTargetWorker.set_worker_identify(szWorkerIdentify);
            oTargetWorker.set_node_type(GetNodeType());
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
            oOutMsgBody.mutable_rsp_result()->set_msg("OK");
            oOutMsgBody.set_data(oTargetWorker.SerializeAsString());
            oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
            oOutMsgHead.set_seq(GetSequence());
            oOutMsgHead.set_len(oOutMsgBody.ByteSize());
            E_CODEC_STATUS eCodecStatus = pChannel->Send(oOutMsgHead, oOutMsgBody);
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
                DestroyConnect(pChannel);
                return(false);
            }
            return(true);
        }
    }
    return(true);
}

} /* namespace neb */
