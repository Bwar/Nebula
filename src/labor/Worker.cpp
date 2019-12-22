/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 27, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include <algorithm>
#include <sched.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "util/process_helper.h"
#include "util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "Worker.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/ActorBuilder.hpp"

namespace neb
{

Worker::Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, Labor::LABOR_TYPE eLaborType)
    : Labor(eLaborType)
{
    m_stWorkerInfo.iControlFd = iControlFd;
    m_stWorkerInfo.iDataFd = iDataFd;
    m_stWorkerInfo.iWorkerIndex = iWorkerIndex;
    m_stWorkerInfo.iWorkerPid = getpid();
    m_stNodeInfo.strWorkPath = strWorkPath;
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
}

Worker::~Worker()
{
    Destroy();
}

void Worker::Run()
{
    LOG4_DEBUG("%s:%d", __FILE__, __LINE__);

    if (!CreateEvents())
    {
        Destroy();
        exit(-2);
    }

    m_pDispatcher->EeventRun();
}

void Worker::OnTerminated(struct ev_signal* watcher)
{
    int iSignum = watcher->signum;
    delete watcher;
    LOG4_FATAL("terminated by signal %d!", iSignum);
    m_pDispatcher->EvBreak();
    Destroy();
    exit(iSignum);
}

bool Worker::CheckParent()
{
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_stWorkerInfo.iWorkerIndex);
        Destroy();
        exit(0);
    }
    MsgBody oMsgBody;
    CJsonObject oJsonLoad;
    m_stWorkerInfo.iConnect = m_pDispatcher->GetConnectionNum();
    m_stWorkerInfo.iClientNum = m_pDispatcher->GetClientNum();
    oJsonLoad.Add("load", int32(m_stWorkerInfo.iConnect + m_pActorBuilder->GetStepNum()));
    oJsonLoad.Add("connect", m_stWorkerInfo.iConnect);
    oJsonLoad.Add("recv_num", m_stWorkerInfo.iRecvNum);
    oJsonLoad.Add("recv_byte", m_stWorkerInfo.iRecvByte);
    oJsonLoad.Add("send_num", m_stWorkerInfo.iSendNum);
    oJsonLoad.Add("send_byte", m_stWorkerInfo.iSendByte);
    oJsonLoad.Add("client", m_stWorkerInfo.iClientNum);
    oMsgBody.set_data(oJsonLoad.ToString());
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    m_pDispatcher->SendTo(m_pManagerControlChannel, CMD_REQ_UPDATE_WORKER_LOAD, GetSequence(), oMsgBody);
    m_stWorkerInfo.iRecvNum = 0;
    m_stWorkerInfo.iRecvByte = 0;
    m_stWorkerInfo.iSendNum = 0;
    m_stWorkerInfo.iSendByte = 0;
    return(true);
}

bool Worker::Init(CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_stWorkerInfo.iWorkerIndex);
    ngx_setproctitle(szProcessName);
    oJsonConf.Get("io_timeout", m_stNodeInfo.dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_stNodeInfo.dStepTimeout))
    {
        m_stNodeInfo.dStepTimeout = 0.5;
    }
    oJsonConf.Get("node_type", m_stNodeInfo.strNodeType);
    oJsonConf.Get("host", m_stNodeInfo.strHostForServer);
    oJsonConf.Get("port", m_stNodeInfo.iPortForServer);
    oJsonConf.Get("data_report", m_stNodeInfo.dDataReportInterval);
    m_oNodeConf = oJsonConf;
    m_oCustomConf = oJsonConf["custom"];
    std::ostringstream oss;
    oss << m_stNodeInfo.strHostForServer << ":" << m_stNodeInfo.iPortForServer << "." << m_stWorkerInfo.iWorkerIndex;
    m_stNodeInfo.strNodeIdentify = std::move(oss.str());

    if (oJsonConf("access_host").size() > 0 && oJsonConf("access_port").size() > 0)
    {
        m_stNodeInfo.bIsAccess = true;
        oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_stNodeInfo.dMsgStatInterval);
        oJsonConf["permission"]["uin_permit"].Get("permit_num", m_stNodeInfo.iMsgPermitNum);
    }
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }

    bool bCpuAffinity = false;
    oJsonConf.Get("cpu_affinity", bCpuAffinity);
    if (bCpuAffinity)
    {
#ifndef __CYGWIN__
        /* get logical cpu number */
        int iCpuNum = sysconf(_SC_NPROCESSORS_CONF);;                               ///< cpu数量
        cpu_set_t stCpuMask;                                                        ///< cpu set
        CPU_ZERO(&stCpuMask);
        CPU_SET(m_stWorkerInfo.iWorkerIndex % iCpuNum, &stCpuMask);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &stCpuMask) == -1)
        {
            LOG4_WARNING("sched_setaffinity failed.");
        }
#endif
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
        std::string strCertFile = m_stNodeInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("cert_file");
        std::string strKeyFile = m_stNodeInfo.strWorkPath + "/" + oJsonConf["with_ssl"]("config_path") + "/" + oJsonConf["with_ssl"]("key_file");
        if (ERR_OK != SocketChannelSslImpl::SslServerCertificate(m_pLogger, strCertFile, strKeyFile))
        {
            LOG4_FATAL("SslServerCertificate() failed!");
            return(false);
        }
#endif
    }

    if (!InitDispatcher() || !InitActorBuilder())
    {
        return(false);
    }

    std::string strChainKey;
    while (oJsonConf["runtime"]["chains"].GetKey(strChainKey))
    {
        std::queue<std::vector<std::string> > queChainBlocks;
        for (int i = 0; i < oJsonConf["runtime"]["chains"][strChainKey].GetArraySize(); ++i)
        {
            std::vector<std::string> vecBlock;
            if (oJsonConf["runtime"]["chains"][strChainKey][i].IsArray())
            {
                for (int j = 0; j < oJsonConf["runtime"]["chains"][strChainKey][i].GetArraySize(); ++j)
                {
                    vecBlock.push_back(std::move(oJsonConf["runtime"]["chains"][strChainKey][i](j)));
                }
            }
            else
            {
                vecBlock.push_back(std::move(oJsonConf["runtime"]["chains"][strChainKey](i)));
            }
            queChainBlocks.push(std::move(vecBlock));
        }
        m_pActorBuilder->AddChainConf(strChainKey, std::move(queChainBlocks));
    }
    return(true);
}

bool Worker::InitLogger(const CJsonObject& oJsonConf)
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
        int32 iMaxLogLineLen = 1024;
        int32 iLogLevel = 0;
        int32 iNetLogLevel = 0;
        std::string strLogname = m_stNodeInfo.strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        oJsonConf.Get("log_max_line_len", iMaxLogLineLen);
        oJsonConf.Get("net_log_level", iNetLogLevel);
        oJsonConf.Get("log_level", iLogLevel);
        m_pLogger = std::make_shared<neb::NetLogger>(strLogname, iLogLevel, iMaxLogFileSize, iMaxLogFileNum, iMaxLogLineLen, this);
        m_pLogger->SetNetLogLevel(iNetLogLevel);
        LOG4_NOTICE("%s program begin...", getproctitle());
        return(true);
    }
}

bool Worker::InitDispatcher()
{
    if (NewDispatcher())
    {
        return(m_pDispatcher->Init());
    }
    return(false);
}

bool Worker::InitActorBuilder()
{
    if (NewActorBuilder())
    {
        return(m_pActorBuilder->Init(
                m_oNodeConf["load_config"]["worker"]["boot_load"],
                m_oNodeConf["load_config"]["worker"]["dynamic_loading"]));
    }
    return(false);
}

bool Worker::NewDispatcher()
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
    return(true);
}

bool Worker::NewActorBuilder()
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
    return(true);
}

bool Worker::CreateEvents()
{
    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* signal_watcher = (ev_signal*)malloc(sizeof(ev_signal));
    signal_watcher->data = (void*)this;
    m_pDispatcher->AddEvent(signal_watcher, Dispatcher::SignalCallback, SIGINT);
    AddPeriodicTaskEvent();

    // 注册网络IO事件
    m_pManagerDataChannel = m_pDispatcher->CreateSocketChannel(m_stWorkerInfo.iDataFd, CODEC_NEBULA);
    m_pDispatcher->SetChannelStatus(m_pManagerDataChannel, CHANNEL_STATUS_ESTABLISHED);
    m_pManagerControlChannel = m_pDispatcher->CreateSocketChannel(m_stWorkerInfo.iControlFd, CODEC_NEBULA);
    m_pDispatcher->SetChannelStatus(m_pManagerControlChannel, CHANNEL_STATUS_ESTABLISHED);
    m_pDispatcher->AddIoReadEvent(m_pManagerDataChannel);
    m_pDispatcher->AddIoReadEvent(m_pManagerControlChannel);
    return(true);
}

void Worker::Destroy()
{
    LOG4_TRACE(" ");

#ifdef WITH_OPENSSL
    SocketChannelSslImpl::SslFree();
#endif
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
    if (m_pErrBuff != nullptr)
    {
        free(m_pErrBuff);
        m_pErrBuff = nullptr;
    }
}

bool Worker::AddPeriodicTaskEvent()
{
    LOG4_TRACE(" ");
    ev_timer* timeout_watcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("malloc timeout_watcher error!");
        return(false);
    }
    timeout_watcher->data = (void*)this->GetDispatcher();
    m_pDispatcher->AddEvent(timeout_watcher, Dispatcher::PeriodicTaskCallback, NODE_BEAT);
    return(true);
}

time_t Worker::GetNowTime() const
{
    return(m_pDispatcher->GetNowTime());
}

const CJsonObject& Worker::GetNodeConf() const
{
    return(m_oNodeConf);
}

void Worker::SetNodeConf(const CJsonObject& oJsonConf)
{
    m_oNodeConf = oJsonConf;
}

const NodeInfo& Worker::GetNodeInfo() const
{
    return(m_stNodeInfo);
}

void Worker::SetNodeId(uint32 uiNodeId)
{
    m_stNodeInfo.uiNodeId = uiNodeId;
}

bool Worker::AddNetLogMsg(const MsgBody& oMsgBody)
{
    // 此函数不能写日志，不然可能会导致写日志函数与此函数无限递归
    m_pActorBuilder->AddNetLogMsg(oMsgBody);
    return(true);
}

const WorkerInfo& Worker::GetWorkerInfo() const
{
    return(m_stWorkerInfo);
}

const CJsonObject& Worker::GetCustomConf() const
{
    return(m_oCustomConf);
}

std::shared_ptr<SocketChannel> Worker::GetManagerControlChannel()
{
    return(m_pManagerControlChannel);
}

bool Worker::SetCustomConf(const CJsonObject& oJsonConf)
{
    m_oCustomConf = oJsonConf;
    return(m_oNodeConf.Replace("custom", oJsonConf));
}

bool Worker::WithSsl()
{
    if (m_oNodeConf["with_ssl"]("config_path").length() > 0)
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
