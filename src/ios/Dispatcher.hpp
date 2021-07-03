/*******************************************************************************
 * Project:  Nebula
 * @file     Dispatcher.hpp
 * @brief    事件管理、事件分发
 * @author   Bwar
 * @date:    2019年9月7日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_IOS_DISPATCHER_HPP_
#define SRC_IOS_DISPATCHER_HPP_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include "ev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#include <string>
#include <unordered_map>
#include <sstream>
#include <memory>

#include "util/process_helper.h"
#include "pb/msg.pb.h"
#include "labor/Labor.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/SelfChannel.hpp"
#include "logger/NetLogger.hpp"
#include "Nodes.hpp"

namespace neb
{

class Labor;
class Manager;
class Worker;
class LoadStress;  // not in Nebula project
class Actor;
class ActorBuilder;
struct tagClientConnWatcherData;

typedef void (*signal_callback)(struct ev_loop*,ev_signal*,int);
typedef void (*timer_callback)(struct ev_loop*,ev_timer*,int);
typedef void (*idle_callback)(struct ev_loop*,ev_idle*,int);

class Dispatcher
{
public:
    struct tagClientConnWatcherData
    {
        char* pAddr;
        Dispatcher* pDispatcher;     // 不在结构体析构时回收

        tagClientConnWatcherData() : pAddr(NULL), pDispatcher(nullptr)
        {
            pAddr = (char*)malloc(gc_iAddrLen);
        }

        ~tagClientConnWatcherData()
        {
            free(pAddr);
            pAddr = nullptr;
        }
    };

    Dispatcher(Labor* pLabor, std::shared_ptr<NetLogger> pLogger);
    virtual ~Dispatcher();
    bool Init();

public:
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);

    bool OnIoRead(std::shared_ptr<SocketChannel> pChannel);
    bool DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel);
    bool DataFetchAndHandle(std::shared_ptr<SocketChannel> pChannel);
    bool FdTransfer(int iFd);
    bool OnIoWrite(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoError(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoTimeout(std::shared_ptr<SocketChannel> pChannel);
    bool OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher);

    template <typename ...Targs>
    void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

    void EventRun();

public:
    bool AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout = 1.0);
    template <typename ...Targs>
    bool SendToSelf(Targs&&... args);
    template <typename ...Targs>
    bool SendTo(std::shared_ptr<SocketChannel> pChannel, Targs&&... args);
    template <typename ...Targs>
    bool SendTo(const std::string& strIdentify, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args);
    template <typename ...Targs>
    bool SendTo(const std::string& strHost, int iPort, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args);
    template <typename ...Targs>
    bool AutoSend(const std::string& strIdentify, const std::string& strHost,
            int iPort, int iRemoteWorkerIndex, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args);
    template <typename ...Targs>
    bool SendRoundRobin(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args);
    template <typename ...Targs>
    bool SendOriented(const std::string& strNodeType, E_CODEC_TYPE eCodecType,
            bool bWithSsl, bool bPipeline, uint32 uiFactor, Targs&&... args);
    template <typename ...Targs>
    bool SendOriented(const std::string& strNodeType, E_CODEC_TYPE eCodecType,
            bool bWithSsl, bool bPipeline, const std::string& strFactor, Targs&&... args);
    template <typename ...Targs>
    bool Broadcast(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args);
    bool AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);
    bool SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    std::shared_ptr<SocketChannel> StressSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    // SendTo() for unix domain socket
    bool SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendTo(int iFd, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    std::shared_ptr<SocketChannel> GetLastActivityChannel();
    bool Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);
    bool DiscardNamedChannel(const std::string& strIdentify);
    Codec* SwitchCodec(std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eCodecType, bool bIsUpgrade = false);

public:
    void SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify);
    bool AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel);
    void DelNamedSocketChannel(const std::string& strIdentify);
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData);
    bool IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType);

    time_t GetNowTime() const
    {
        return((time_t)ev_now(m_loop));
    }
    long GetNowTimeMs() const
    {
        return((long)ev_now(m_loop) * 1000);
    }
    std::shared_ptr<SocketChannel> CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsClient = false, bool bWithSsl = false);
    bool DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    bool CreateListenFd(const std::string& strHost, int32 iPort, int& iFd, int& iFamily);
    std::shared_ptr<SocketChannel> GetChannel(int iFd);
    int SendFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType);

protected:
    void Destroy();
    bool AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddEvent(ev_signal* signal_watcher, signal_callback pFunc, int iSignum);
    bool AddEvent(ev_timer* timer_watcher, timer_callback pFunc, ev_tstamp dTimeout);
    bool AddEvent(ev_idle* idle_watcher, idle_callback pFunc);
    bool RefreshEvent(ev_timer* timer_watcher, ev_tstamp dTimeout);
    bool DelEvent(ev_io* io_watcher);
    bool DelEvent(ev_timer* timer_watcher);
    int32 GetConnectionNum() const;
    int32 GetClientNum() const;
    void SetChannelStatus(std::shared_ptr<SocketChannel> pChannel, E_CHANNEL_STATUS eStatus);
    bool AddClientConnFrequencyTimeout(const char* pAddr, ev_tstamp dTimeout = 60.0);
    bool AcceptFdAndTransfer(int iFd, int iFamily = AF_INET);
    bool AcceptServerConn(int iFd);
    void CheckFailedNode();
    void EvBreak();
    bool Deliver(std::shared_ptr<SelfChannel> pSelfChannel);
    bool Deliver(std::shared_ptr<SelfChannel> pSelfChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, uint32 uiStepSeq = 0);
    bool Deliver(std::shared_ptr<SelfChannel> pSelfChannel, const HttpMsg& oHttpMsg, uint32 uiStepSeq = 0);
    bool Deliver(std::shared_ptr<SelfChannel> pSelfChannel, const RedisMsg& oRedisMsg, uint32 uiStepSeq = 0);
    bool Deliver(std::shared_ptr<SelfChannel> pSelfChannel, const char* pRaw, uint32 uiRawSize, uint32 uiStepSeq = 0);

private:
    char* m_pErrBuff;
    Labor* m_pLabor;
    struct ev_loop* m_loop;
    int32 m_iClientNum;
    time_t m_lLastCheckNodeTime;
    std::shared_ptr<NetLogger> m_pLogger;
    std::unique_ptr<Nodes> m_pSessionNode;
    std::shared_ptr<SocketChannel> m_pLastActivityChannel;  // 最近一个发送或接收过数据的channel

    // Channel
    std::unordered_map<int32, std::shared_ptr<SocketChannel> > m_mapSocketChannel;

    /* named Channel */
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<SocketChannel> > > m_mapNamedSocketChannel;      ///< key为Identify，连接存在时，if(http连接)set.size()>=1;else set.size()==1;
    std::unordered_map<int32, std::shared_ptr<SocketChannel> > m_mapLoaderAndWorkerChannel;     ///< Loader和Worker之间通信通道
    std::unordered_map<int32, std::shared_ptr<SocketChannel> >::iterator m_iterLoaderAndWorkerChannel;

    std::unordered_map<std::string, uint32> m_mapClientConnFrequency;   ///< 客户端连接频率

    friend class Manager;
    friend class Worker;
    friend class ActorBuilder;
    friend class LoadStress;
};

template <typename ...Targs>
void Dispatcher::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
bool Dispatcher::SendToSelf(Targs&&... args)
{
    auto pSelfChannel = std::make_shared<SelfChannel>();
    return(Deliver(pSelfChannel, std::forward<Targs>(args)...));
}

template <typename ...Targs>
bool Dispatcher::SendTo(std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    if (pChannel->GetCodecType() == CODEC_DIRECT)
    {
        auto pSelfChannel = std::dynamic_pointer_cast<SelfChannel>(pChannel);
        if (pSelfChannel == nullptr)
        {
            LOG4_ERROR("channel is not a self channel.");
            return(false);
        }
        return(Deliver(pSelfChannel, std::forward<Targs>(args)...));
    }
    E_CODEC_STATUS eStatus = pChannel->m_pImpl->Send(std::forward<Targs>(args)...);
    m_pLabor->IoStatAddSendNum(pChannel->GetFd());
    m_pLastActivityChannel = pChannel;
    switch (eStatus)
    {
        case CODEC_STATUS_OK:
            return(true);
        case CODEC_STATUS_PAUSE:
        case CODEC_STATUS_WANT_WRITE:
        case CODEC_STATUS_PART_OK:
            AddIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_WANT_READ:
            RemoveIoWriteEvent(pChannel);
            return(true);
        case CODEC_STATUS_EOF:      // a case: http1.0 respone and close
            LOG4_TRACE("eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            DiscardSocketChannel(pChannel);
            return(true);
        default:
            LOG4_TRACE("eStatus = %d, %s", eStatus, pChannel->GetIdentify().c_str());
            DiscardSocketChannel(pChannel);
            return(false);
    }
}

template <typename ...Targs>
bool Dispatcher::SendTo(const std::string& strIdentify, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args)
{
    LOG4_TRACE("identify: %s", strIdentify.c_str());
    // 将strIdentify分割的功能只在此SendTo()函数内两处调用，定义为Dispatcher的成员函数语义上不太合适，故定义lambda表达式
    auto split = [](const std::string& strIdentify, std::string& strHost, int& iPort, int& iWorkerIndex, std::string& strError)->bool
    {
        size_t iPosIpPortSeparator = strIdentify.rfind(':');
        size_t iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
        if (iPosIpPortSeparator == std::string::npos)
        {
            return(false);
        }
        strHost = strIdentify.substr(0, iPosIpPortSeparator);
        std::string strPort;
        if (iPosPortWorkerIndexSeparator != std::string::npos && iPosPortWorkerIndexSeparator > iPosIpPortSeparator)
        {
            strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
            iPort = atoi(strPort.c_str());
            if (iPort == 0)
            {
                return(false);
            }
            std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
            if (strWorkerIndex.size() > 0)
            {
                iWorkerIndex = atoi(strWorkerIndex.c_str());
                if (iWorkerIndex > 200)
                {
                    strError = "worker index must smaller than 200";
                    return(false);
                }
            }
        }
        else
        {
            strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
            iPort = atoi(strPort.c_str());
            if (iPort == 0)
            {
                return(false);
            }
        }
        return(true);
    };

    auto named_iter = m_mapNamedSocketChannel.find(strIdentify);
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        std::string strError;
        std::string strHost;
        int iPort = 0;
        int iWorkerIndex = -1;
        if (!split(strIdentify, strHost, iPort, iWorkerIndex, strError))
        {
            LOG4_ERROR("%s", strError.c_str());
            return(false);
        }
        return(AutoSend(strIdentify, strHost, iPort, iWorkerIndex, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
    }
    else
    {
        if (named_iter->second.empty())
        {
            std::string strError;
            std::string strHost;
            int iPort = 0;
            int iWorkerIndex = -1;
            if (!split(strIdentify, strHost, iPort, iWorkerIndex, strError))
            {
                LOG4_ERROR("%s", strError.c_str());
                return(false);
            }
            return(AutoSend(strIdentify, strHost, iPort, iWorkerIndex, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
        }
        else
        {
            auto channel_iter = named_iter->second.begin();
            bool bResult = SendTo((*channel_iter), std::forward<Targs>(args)...);
            if (!bPipeline && bResult)
            {
                named_iter->second.erase(channel_iter);
            }
            return(bResult);
        }
    }
}

template <typename ...Targs>
bool Dispatcher::SendTo(const std::string& strHost, int iPort, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args)
{
    LOG4_TRACE("host %s port %d", strHost.c_str(), iPort);
    std::ostringstream ossIdentify;
    ossIdentify << strHost << ":" << iPort;
    auto named_iter = m_mapNamedSocketChannel.find(ossIdentify.str());
    if (named_iter == m_mapNamedSocketChannel.end())
    {
        LOG4_TRACE("no channel match %s.", ossIdentify.str().c_str());
        return(AutoSend(ossIdentify.str(), strHost, iPort, 0, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
    }
    else
    {
        auto channel_iter = named_iter->second.begin();
        if (channel_iter == named_iter->second.end())
        {
            return(AutoSend(ossIdentify.str(), strHost, iPort, 0, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
        }
        bool bResult = SendTo((*channel_iter), std::forward<Targs>(args)...);
        if (!bPipeline && bResult)
        {
            named_iter->second.erase(channel_iter);
        }
        return(bResult);
    }
}

template <typename ...Targs>
bool Dispatcher::AutoSend(
        const std::string& strIdentify, const std::string& strHost, int iPort,
        int iRemoteWorkerIndex, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args)
{
    LOG4_TRACE("%s", strIdentify.c_str());
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
    std::shared_ptr<SocketChannel> pChannel = CreateSocketChannel(iFd, eCodecType, true, bWithSsl);
    if (nullptr != pChannel)
    {
        connect(iFd, pAddrCurrent->ai_addr, pAddrCurrent->ai_addrlen);
        freeaddrinfo(pAddrResult);           /* No longer needed */
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->m_pImpl->SetIdentify(strIdentify);
        pChannel->m_pImpl->SetRemoteAddr(strHost);
        pChannel->m_pImpl->SetPipeline(bPipeline);
        E_CODEC_STATUS eCodecStatus = pChannel->m_pImpl->Send(std::forward<Targs>(args)...);
        m_pLabor->IoStatAddSendNum(pChannel->GetFd());
        m_pLastActivityChannel = pChannel;
        if (CODEC_STATUS_OK != eCodecStatus
                && CODEC_STATUS_PAUSE != eCodecStatus
                && CODEC_STATUS_WANT_WRITE != eCodecStatus
                && CODEC_STATUS_WANT_READ != eCodecStatus)
        {
            DiscardSocketChannel(pChannel);
        }

        pChannel->m_pImpl->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
        pChannel->m_pImpl->SetRemoteWorkerIndex(iRemoteWorkerIndex);
        if (bPipeline)
        {
            AddNamedSocketChannel(strIdentify, pChannel);
        }
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        freeaddrinfo(pAddrResult);           /* No longer needed */
        close(iFd);
        return(false);
    }
}

template <typename ...Targs>
bool Dispatcher::SendRoundRobin(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, strOnlineNode))
    {
        return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
        }
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template <typename ...Targs>
bool Dispatcher::SendOriented(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, uint32 uiFactor, Targs&&... args)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, uiFactor, strOnlineNode))
    {
        return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
        }
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template <typename ...Targs>
bool Dispatcher::SendOriented(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, const std::string& strFactor, Targs&&... args)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::string strOnlineNode;
    if (m_pSessionNode->GetNode(strNodeType, strFactor, strOnlineNode))
    {
        return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
    }
    else
    {
        LOG4_TRACE("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
        if (m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
        {
            return(SendTo(strOnlineNode, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...));
        }
        LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        return(false);
    }
}

template <typename ...Targs>
bool Dispatcher::Broadcast(const std::string& strNodeType, E_CODEC_TYPE eCodecType, bool bWithSsl, bool bPipeline, Targs&&... args)
{
    LOG4_TRACE("node_type: %s", strNodeType.c_str());
    std::unordered_set<std::string> setOnlineNodes;
    if (m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
    {
        bool bSendResult = false;
        for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
        {
            bSendResult |= SendTo(*node_iter, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...);
        }
        return(bSendResult);
    }
    else
    {
        if ("BEACON" == strNodeType)
        {
            LOG4_TRACE("no beacon config.");
        }
        else
        {
            LOG4_TRACE("node type \"%s\" not found, go to SplitAddAndGetNode.", strNodeType.c_str());
            std::string strOnlineNode;
            if (m_pSessionNode->SplitAddAndGetNode(strNodeType, strOnlineNode))
            {
                if (m_pSessionNode->GetNode(strNodeType, setOnlineNodes))
                {
                    bool bSendResult = false;
                    for (auto node_iter = setOnlineNodes.begin(); node_iter != setOnlineNodes.end(); ++node_iter)
                    {
                        bSendResult |= SendTo(*node_iter, eCodecType, bWithSsl, bPipeline, std::forward<Targs>(args)...);
                    }
                    return(bSendResult);
                }
            }
            LOG4_ERROR("no online node match node_type \"%s\"", strNodeType.c_str());
        }
        return(false);
    }
}

} /* namespace neb */

#endif /* SRC_IOS_DISPATCHER_HPP_ */
