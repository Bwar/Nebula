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
#include "Loader.hpp"
#include "channel/SocketChannel.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/ActorBuilder.hpp"
#include "actor/step/Step.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

Manager::Manager(const std::string& strConfFile)
    : Labor(LABOR_MANAGER),
      m_uiSequence(0), m_pDispatcher(nullptr), m_pActorBuilder(nullptr)
{
    if (strConfFile == "")
    {
        std::cerr << "error: no config file!" << std::endl;
        exit(1);
    }

    m_stNodeInfo.strConfFile = strConfFile;
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
    if (!GetConf())
    {
        std::cerr << "GetConf() error!" << std::endl;
        exit(2);
    }
    ngx_setproctitle(m_oCurrentConf("server_name").c_str());
    daemonize(m_oCurrentConf("server_name").c_str());
    if (!Init())
    {
        exit(3);
    }
    m_stManagerInfo.iWorkerBeat = (gc_iBeatInterval * 2) + 1;
    CreateEvents();
    CreateLoader();
    CreateWorker();
}

Manager::~Manager()
{
    Destroy();
}

void Manager::OnTerminated(struct ev_signal* watcher)
{
    LOG4_WARNING("%s terminated by signal %d!", m_oCurrentConf("server_name").c_str(), watcher->signum);
    int iSignum = watcher->signum;
    delete watcher;
    LOG4_FATAL("terminated by signal %d!", iSignum);
    m_pDispatcher->EvBreak();
    Destroy();
    exit(iSignum);
}

void Manager::OnChildTerminated(struct ev_signal* watcher)
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
}

void Manager::Run()
{
    LOG4_TRACE(" ");
    m_pDispatcher->EeventRun();
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
        int32 iMaxLogLineLen = 1024;
        std::string strLoggingHost;
        std::string strLogname = m_stNodeInfo.strWorkPath + std::string("/")
                        + oJsonConf("log_path") + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << m_stNodeInfo.strHostForServer << ":" << m_stNodeInfo.iPortForServer;
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_max_line_len", iMaxLogLineLen);
        oJsonConf.Get("log_level", iLogLevel);
        m_pLogger = std::make_shared<NetLogger>(strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, iMaxLogLineLen, this);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        LOG4_NOTICE("%s program begin, and work path %s...", oJsonConf("server_name").c_str(), m_stNodeInfo.strWorkPath.c_str());
        return(true);
    }
}

bool Manager::InitDispatcher()
{
    try
    {
        m_pDispatcher = new Dispatcher(this, m_pLogger);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new Dispatcher error: %s", e.what());
        return(false);
    }
    return(m_pDispatcher->Init());
}

bool Manager::InitActorBuilder()
{
    try
    {
        m_pActorBuilder = new ActorBuilder(this, m_pLogger);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new ActorBuilder error: %s", e.what());
        return(false);
    }
    if (!m_pActorBuilder->Init(
            m_oCurrentConf["load_conf"]["manager"]["boot_load"],
            m_oCurrentConf["load_conf"]["manager"]["dynamic_loading"]))
    {
        LOG4_ERROR("ActorBuilder->Init() failed!");
        return(false);
    }
    return(true);
}

bool Manager::AddNetLogMsg(const MsgBody& oMsgBody)
{
    if (std::string("BEACON") != m_stNodeInfo.strNodeType
            && std::string("LOGGER") != m_stNodeInfo.strNodeType)
    {
        m_pActorBuilder->AddNetLogMsg(oMsgBody);
    }
    return(true);
}

time_t Manager::GetNowTime() const
{
    return(m_pDispatcher->GetNowTime());
}

bool Manager::GetConf()
{
    if (m_stNodeInfo.strWorkPath.length() == 0)
    {
        char szFilePath[256] = {0};
        if (getcwd(szFilePath, sizeof(szFilePath)))
        {
            m_stNodeInfo.strWorkPath = szFilePath;
        }
        else
        {
            std::cerr << "failed to open work dir: " << m_stNodeInfo.strWorkPath << std::endl;
            return(false);
        }
    }
    m_oLastConf = m_oCurrentConf;
    //snprintf(szFileName, sizeof(szFileName), "%s/%s", m_strWorkPath.c_str(), m_strConfFile.c_str());
    std::ifstream fin(m_stNodeInfo.strConfFile.c_str());
    if (fin.good())
    {
        std::stringstream ssContent;
        ssContent << fin.rdbuf();
        if (!m_oCurrentConf.Parse(ssContent.str()))
        {
            ssContent.str("");
            fin.close();
            m_oCurrentConf = m_oLastConf;
            std::cerr << "failed to parse json file " << m_stNodeInfo.strConfFile << std::endl;
            return(false);
        }
        ssContent.str("");
        fin.close();
    }
    else
    {
        std::cerr << "failed to open file " << m_stNodeInfo.strConfFile << std::endl;
        return(false);
    }

    if (m_oLastConf.ToString() != m_oCurrentConf.ToString())
    {
        m_oCurrentConf.Get("io_timeout", m_stNodeInfo.dIoTimeout);
        m_oCurrentConf.Get("data_report", m_stNodeInfo.dDataReportInterval);
        if (m_oLastConf.ToString().length() == 0)
        {
            m_stNodeInfo.uiWorkerNum = strtoul(m_oCurrentConf("worker_num").c_str(), NULL, 10);
            m_oCurrentConf.Get("node_type", m_stNodeInfo.strNodeType);
            m_oCurrentConf.Get("host", m_stNodeInfo.strHostForServer);
            m_oCurrentConf.Get("port", m_stNodeInfo.iPortForServer);
            m_oCurrentConf.Get("access_host", m_stNodeInfo.strHostForClient);
            m_oCurrentConf.Get("access_port", m_stNodeInfo.iPortForClient);
            m_oCurrentConf.Get("gateway", m_stNodeInfo.strGateway);
            m_oCurrentConf.Get("gateway_port", m_stNodeInfo.iGatewayPort);
            m_stNodeInfo.strNodeIdentify = m_stNodeInfo.strHostForServer + std::string(":") + std::to_string(m_stNodeInfo.iPortForServer);
        }
        int32 iCodec;
        if (m_oCurrentConf.Get("access_codec", iCodec))
        {
            m_stNodeInfo.eCodec = E_CODEC_TYPE(iCodec);
        }
        m_oCurrentConf["permission"]["addr_permit"].Get("stat_interval", m_stNodeInfo.dAddrStatInterval);
        m_oCurrentConf["permission"]["addr_permit"].Get("permit_num", m_stNodeInfo.iAddrPermitNum);
    }
    return(true);
}

bool Manager::Init()
{
    if (!InitLogger(m_oCurrentConf) || !InitDispatcher() || !InitActorBuilder())
    {
        return(false);
    }

    if (m_stNodeInfo.strHostForClient.size() > 0 && m_stNodeInfo.iPortForClient > 0)
    {
        // 接入节点才需要监听客户端连接
        if (!m_pDispatcher->CreateListenFd(m_stNodeInfo.strHostForClient,
                m_stNodeInfo.iPortForClient, m_stManagerInfo.iC2SListenFd,
                m_stManagerInfo.iC2SFamily))
        {
            return(false);
        }
    }


    if (!m_pDispatcher->CreateListenFd(m_stNodeInfo.strHostForServer,
            m_stNodeInfo.iPortForServer, m_stManagerInfo.iS2SListenFd,
            m_stManagerInfo.iS2SFamily))
    {
        return(false);
    }

    // 创建到beacon的连接信息
    for (int i = 0; i < m_oCurrentConf["beacon"].GetArraySize(); ++i)
    {
        std::string strIdentify = m_oCurrentConf["beacon"][i]("host") + std::string(":")
            + m_oCurrentConf["beacon"][i]("port") + std::string(".1");     // BeaconServer只有一个Worker
        m_pDispatcher->AddNodeIdentify(std::string("BEACON"), strIdentify);
    }

    return(true);
}

void Manager::Destroy()
{
    LOG4_TRACE(" ");
    if (m_pPeriodicTaskWatcher != NULL)
    {
        m_pDispatcher->DelEvent(m_pPeriodicTaskWatcher);
        free(m_pPeriodicTaskWatcher);
        m_pPeriodicTaskWatcher = NULL;
    }
    if (m_pDispatcher != nullptr)
    {
        delete m_pDispatcher;
        m_pDispatcher = nullptr;
    }
    if (m_pActorBuilder != nullptr)
    {
        delete m_pActorBuilder;
        m_pActorBuilder = nullptr;
    }

    if (m_pErrBuff != NULL)
    {
        free(m_pErrBuff);
        m_pErrBuff = NULL;
    }
}

bool Manager::CreateEvents()
{
    LOG4_TRACE(" ");
    std::shared_ptr<SocketChannel> pChannelListen = NULL;
    if (m_stManagerInfo.iC2SListenFd > 2)
    {
        LOG4_DEBUG("C2SListenFd[%d]", m_stManagerInfo.iC2SListenFd);
        pChannelListen = m_pDispatcher->CreateSocketChannel(m_stManagerInfo.iC2SListenFd, m_stNodeInfo.eCodec);
        m_pDispatcher->SetChannelStatus(pChannelListen, CHANNEL_STATUS_ESTABLISHED);
        m_pDispatcher->AddIoReadEvent(pChannelListen);
    }
    LOG4_DEBUG("S2SListenFd[%d]", m_stManagerInfo.iS2SListenFd);
    pChannelListen = m_pDispatcher->CreateSocketChannel(m_stManagerInfo.iS2SListenFd, CODEC_NEBULA);
    m_pDispatcher->SetChannelStatus(pChannelListen, CHANNEL_STATUS_ESTABLISHED);
    m_pDispatcher->AddIoReadEvent(pChannelListen);

    ev_signal* child_signal_watcher = new ev_signal();
    child_signal_watcher->data = (void*)this;
    m_pDispatcher->AddEvent(child_signal_watcher, Dispatcher::SignalCallback, SIGCHLD);

    ev_signal* ill_signal_watcher = new ev_signal();
    ill_signal_watcher->data = (void*)this;
    m_pDispatcher->AddEvent(ill_signal_watcher, Dispatcher::SignalCallback, SIGILL);

    ev_signal* bus_signal_watcher = new ev_signal();
    bus_signal_watcher->data = (void*)this;
    m_pDispatcher->AddEvent(bus_signal_watcher, Dispatcher::SignalCallback, SIGBUS);

    ev_signal* fpe_signal_watcher = new ev_signal();
    fpe_signal_watcher->data = (void*)this;
    m_pDispatcher->AddEvent(fpe_signal_watcher, Dispatcher::SignalCallback, SIGFPE);

    bool bDirectToLoader = false;
    m_oCurrentConf.Get("new_client_to_loader", bDirectToLoader);
    m_pSessionManager = std::dynamic_pointer_cast<SessionManager>(
            m_pActorBuilder->MakeSharedSession(nullptr, "neb::SessionManager", bDirectToLoader));
    AddPeriodicTaskEvent();

    return(true);
}

void Manager::CreateLoader()
{
    bool bWithLoader = false;
    m_oCurrentConf.Get("with_loader", bWithLoader);
    if (!bWithLoader)
    {
        return;
    }
    LOG4_TRACE(" ");
    int iControlFds[2];
    int iDataFds[2];
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
    }
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
    }

    int iPid = fork();
    if (iPid == 0)   // 子进程
    {
        close(m_stManagerInfo.iS2SListenFd);
        if (m_stManagerInfo.iC2SListenFd > 2)
        {
            close(m_stManagerInfo.iC2SListenFd);
        }
        close(iControlFds[0]);
        close(iDataFds[0]);
        x_sock_set_block(iControlFds[1], 0);
        x_sock_set_block(iDataFds[1], 0);
        Loader oLoader(m_stNodeInfo.strWorkPath, iControlFds[1], iDataFds[1], 0);
        if (!oLoader.Init(m_oCurrentConf))
        {
            exit(3);
        }
        oLoader.Run();
        exit(-2);
    }
    else if (iPid > 0)   // 父进程
    {
        close(iControlFds[1]);
        close(iDataFds[1]);
        x_sock_set_block(iControlFds[0], 0);
        x_sock_set_block(iDataFds[0], 0);
        m_pSessionManager->AddLoaderInfo(0, iPid, iControlFds[0], iDataFds[0]);
        std::shared_ptr<SocketChannel> pChannelData = m_pDispatcher->CreateSocketChannel(iControlFds[0], CODEC_NEBULA);
        std::shared_ptr<SocketChannel> pChannelControl = m_pDispatcher->CreateSocketChannel(iDataFds[0], CODEC_NEBULA);
        m_pDispatcher->SetChannelStatus(pChannelData, CHANNEL_STATUS_ESTABLISHED);
        m_pDispatcher->SetChannelStatus(pChannelControl, CHANNEL_STATUS_ESTABLISHED);
        m_pDispatcher->AddIoReadEvent(pChannelData);
        m_pDispatcher->AddIoReadEvent(pChannelControl);
        m_pSessionManager->SendOnlineNodesToWorker();
    }
    else
    {
        LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
    }
}

void Manager::CreateWorker()
{
    LOG4_TRACE(" ");
    int iPid = 0;

    for (unsigned int i = 1; i <= m_stNodeInfo.uiWorkerNum; ++i)
    {
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }

        iPid = fork();
        if (iPid == 0)   // 子进程
        {
            close(m_stManagerInfo.iS2SListenFd);
            if (m_stManagerInfo.iC2SListenFd > 2)
            {
                close(m_stManagerInfo.iC2SListenFd);
            }
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            Worker oWorker(m_stNodeInfo.strWorkPath, iControlFds[1], iDataFds[1], i);
            if (!oWorker.Init(m_oCurrentConf))
            {
                exit(3);
            }
            oWorker.Run();
            exit(-2);
        }
        else if (iPid > 0)   // 父进程
        {
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            m_pSessionManager->AddWorkerInfo(i, iPid, iControlFds[0], iDataFds[0]);
            std::shared_ptr<SocketChannel> pChannelData = m_pDispatcher->CreateSocketChannel(iControlFds[0], CODEC_NEBULA);
            std::shared_ptr<SocketChannel> pChannelControl = m_pDispatcher->CreateSocketChannel(iDataFds[0], CODEC_NEBULA);
            m_pDispatcher->SetChannelStatus(pChannelData, CHANNEL_STATUS_ESTABLISHED);
            m_pDispatcher->SetChannelStatus(pChannelControl, CHANNEL_STATUS_ESTABLISHED);
            m_pDispatcher->AddIoReadEvent(pChannelData);
            m_pDispatcher->AddIoReadEvent(pChannelControl);
            m_pSessionManager->NewSocketWhenWorkerCreated(iDataFds[0]);
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }
    }
}

bool Manager::RestartWorker(int iDeathPid)
{
    LOG4_DEBUG("%d", iDeathPid);
    int iWorkerIndex = 0;
    int iNewPid = 0;
    Labor::LABOR_TYPE eLaborType;
    if (m_pSessionManager->WorkerDeath(iDeathPid, iWorkerIndex, eLaborType))
    {
        int iControlFds[2];
        int iDataFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iControlFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iDataFds) < 0)
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }

        iNewPid = fork();
        if (iNewPid == 0)   // 子进程
        {
            close(m_stManagerInfo.iS2SListenFd);
            if (m_stManagerInfo.iC2SListenFd > 2)
            {
                close(m_stManagerInfo.iC2SListenFd);
            }
            close(iControlFds[0]);
            close(iDataFds[0]);
            x_sock_set_block(iControlFds[1], 0);
            x_sock_set_block(iDataFds[1], 0);
            if (Labor::LABOR_LOADER == eLaborType)
            {
                Loader oLoader(m_stNodeInfo.strWorkPath, iControlFds[1], iDataFds[1], 0);
                if (!oLoader.Init(m_oCurrentConf))
                {
                    exit(-1);
                }
                oLoader.Run();
            }
            else
            {
                Worker oWorker(m_stNodeInfo.strWorkPath, iControlFds[1], iDataFds[1], iWorkerIndex);
                if (!oWorker.Init(m_oCurrentConf))
                {
                    exit(-1);
                }
                oWorker.Run();
            }
            exit(-2);   // 子进程worker没有正常运行
        }
        else if (iNewPid > 0)   // 父进程
        {
            LOG4_INFO("worker %d restart successfully", iWorkerIndex);
            close(iControlFds[1]);
            close(iDataFds[1]);
            x_sock_set_block(iControlFds[0], 0);
            x_sock_set_block(iDataFds[0], 0);
            m_pSessionManager->AddWorkerInfo(iWorkerIndex, iNewPid, iControlFds[0], iDataFds[0]);
            std::shared_ptr<SocketChannel> pChannelData = m_pDispatcher->CreateSocketChannel(iControlFds[0], CODEC_NEBULA);
            std::shared_ptr<SocketChannel> pChannelControl = m_pDispatcher->CreateSocketChannel(iDataFds[0], CODEC_NEBULA);
            m_pDispatcher->SetChannelStatus(pChannelData, CHANNEL_STATUS_ESTABLISHED);
            m_pDispatcher->SetChannelStatus(pChannelControl, CHANNEL_STATUS_ESTABLISHED);
            m_pDispatcher->AddIoReadEvent(pChannelData);
            m_pDispatcher->AddIoReadEvent(pChannelControl);
            m_pSessionManager->SendOnlineNodesToWorker();
            if (Labor::LABOR_LOADER == eLaborType)
            {
                m_pSessionManager->AddLoaderInfo(iWorkerIndex, iNewPid, iControlFds[0], iDataFds[0]);
                m_pSessionManager->NewSocketWhenLoaderCreated();
            }
            else
            {
                m_pSessionManager->AddWorkerInfo(iWorkerIndex, iNewPid, iControlFds[0], iDataFds[0]);
                m_pSessionManager->NewSocketWhenWorkerCreated(iDataFds[0]);
            }
        }
        else
        {
            LOG4_ERROR("error %d: %s", errno, strerror_r(errno, m_pErrBuff, gc_iErrBuffLen));
        }
    }
    return(false);
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
            m_oCurrentConf.Get("log_level", iLogLevel);
            m_oCurrentConf.Get("net_log_level", iNetLogLevel);
            m_pLogger->SetLogLevel(iLogLevel);
            m_pLogger->SetNetLogLevel(iNetLogLevel);
            MsgBody oMsgBody;
            LogLevel oLogLevel;
            oLogLevel.set_log_level(iLogLevel);
            oLogLevel.set_net_log_level(iNetLogLevel);
            oMsgBody.set_data(oLogLevel.SerializeAsString());
            m_pSessionManager->SendToChild(CMD_REQ_SET_LOG_LEVEL, GetSequence(), oMsgBody);
        }

        // 更新动态库配置或重新加载动态库
        if (m_oLastConf["load_config"]["worker"]["dynamic_loading"].ToString()
                != m_oCurrentConf["load_config"]["worker"]["dynamic_loading"].ToString())
        {
            MsgBody oMsgBody;
            oMsgBody.set_data(m_oCurrentConf["load_config"]["worker"]["dynamic_loading"].ToString());
            m_pSessionManager->SendToWorker(CMD_REQ_RELOAD_SO, GetSequence(), oMsgBody);
        }
        if (m_oLastConf["load_config"]["loader"]["dynamic_loading"].ToString()
                != m_oCurrentConf["load_config"]["loader"]["dynamic_loading"].ToString())
        {
            MsgBody oMsgBody;
            oMsgBody.set_data(m_oCurrentConf["load_config"]["loader"]["dynamic_loading"].ToString());
            m_pSessionManager->SendToLoader(CMD_REQ_RELOAD_SO, GetSequence(), oMsgBody);
        }
    }
    else
    {
        LOG4_ERROR("GetConf() error, please check the config file.", "");
    }
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
    m_pPeriodicTaskWatcher->data = (void*)this->GetDispatcher();
    m_pDispatcher->AddEvent(m_pPeriodicTaskWatcher, Dispatcher::PeriodicTaskCallback, NODE_BEAT);
    std::shared_ptr<Step> pReportToBeacon = m_pActorBuilder->MakeSharedStep(
            nullptr, "neb::StepReportToBeacon", NODE_BEAT);
    //pReportToBeacon->Emit();
    return(true);
}

const CJsonObject& Manager::GetNodeConf() const
{
    return(m_oCurrentConf);
}

void Manager::SetNodeConf(const CJsonObject& oNodeConf)
{
    m_oCurrentConf = oNodeConf;
}

const NodeInfo& Manager::GetNodeInfo() const
{
    return(m_stNodeInfo);
}

void Manager::SetNodeId(uint32 uiNodeId)
{
    m_stNodeInfo.uiNodeId = uiNodeId;
}

const Manager::tagManagerInfo& Manager::GetManagerInfo() const
{
    return(m_stManagerInfo);
}

} /* namespace neb */
