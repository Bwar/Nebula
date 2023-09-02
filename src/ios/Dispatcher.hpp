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
#include <list>
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
class CodecFactory;
class LoadStress;  // not in Nebula project
class Actor;
class ActorBuilder;
class CmdFdTransfer;
class PollingWatcher;
struct tagClientConnWatcherData;
template<typename T> class IO;

typedef void (*signal_callback)(struct ev_loop*,ev_signal*,int);
typedef void (*timer_callback)(struct ev_loop*,ev_timer*,int);
typedef void (*idle_callback)(struct ev_loop*,ev_idle*,int);
typedef void (*async_callback)(struct ev_loop*,ev_async*,int);
typedef void (*prepare_callback)(struct ev_loop*, ev_prepare*, int);
typedef void (*check_callback)(struct ev_loop*, ev_check*, int);

enum E_DISPATCHER
{
    DISPATCH_ROUND_ROBIN = 0,
    DISPATCH_CLIENT_ADDR_HASH = 1,
};

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
    bool Init(uint32 uiUpstreamConnectionPoolSize, uint32 uiMaxSendBuffSize, uint32 uiMaxRecvBuffSize);

public:
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void AsyncCallback(struct ev_loop* loop, struct ev_async* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void PollingCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PrepareCallback(struct ev_loop* loop, struct ev_prepare* watcher, int revents);
    static void CheckCallback(struct ev_loop* loop, struct ev_check* watcher, int revents);
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);

    bool OnIoRead(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoWrite(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoError(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoTimeout(std::shared_ptr<SocketChannel> pChannel);
    bool OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher);
    bool DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel);
    bool ReactSend(std::shared_ptr<SocketChannel> pChannel);
    bool MigrateChannelRecvAndHandle(std::shared_ptr<SocketChannel> pChannel);

    template <typename ...Targs>
    void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

    void EventRun();

public:
    bool AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout = 1.0);
    bool SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    std::shared_ptr<SocketChannel> StressSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    std::shared_ptr<SocketChannel> GetLastActivityChannel();
    bool Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);
    bool DiscardNamedChannel(const std::string& strIdentify);

public:
    void SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify);
    bool AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel);
    bool AdjustNamedSocketChannel(std::shared_ptr<SocketChannel> pChannel);
    std::shared_ptr<SocketChannel> ApplyNamedSocketChannel(const std::string& strIdentify, uint32& uiPoolSize);
    void DelNamedSocketChannel(const std::string& strIdentify);
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void ReplaceNodes(const std::string& strNodeType, const std::set<std::string>& setNodeIdentify);
    void CircuitBreak(const std::string& strIdentify);
    void SetChannelPingStep(int iCodec, const std::string& strStepName);
    void SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData);
    bool IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType);
    bool GetAuth(const std::string& strIdentify, std::string& strAuth, std::string& strPassword);
    std::shared_ptr<ChannelOption> GetChannelOption(const std::string& strIdentify);
    void SetChannelOption(const std::string& strIdentify, const ChannelOption& stOption);

    uint32 GetUpstreamConnectionPoolSize() const
    {
        return(m_uiUpstreamConnectionPoolSize);
    }
    time_t GetNowTime() const
    {
        return((time_t)ev_now(m_loop));
    }
    long GetNowTimeMs() const
    {
        return((long)(ev_now(m_loop) * 1000));
    }
    std::shared_ptr<NetLogger> GetLogger() const
    {
        return(m_pLogger);
    }
    std::shared_ptr<SocketChannel> CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsClient = false, bool bWithSsl = false);
    bool DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    /**
     * @brief migrate socket channel
     * @note
     * 1. only server side socket channel can be migrate. 只有服务端的socket channel才可以迁移。
     * 2. make sure the socket channel that is moved out will no longer send response from outgoing side.
     * 确保不再有来自迁出线程的响应包发送到迁出的socket channel，否则该响应将无法送达。
     */
    bool MigrateSocketChannel(uint32 uiFromLabor, uint32 uiToLabor, std::shared_ptr<SocketChannel> pChannel);
    bool CreateListenFd(const std::string& strHost, int32 iPort, int iBacklog, int& iFd, int& iFamily);
    std::shared_ptr<SocketChannel> GetChannel(int iFd);
    void AddChannelToLoop(std::shared_ptr<SocketChannel> pChannel);
    void AsyncSend(ev_async* pWatcher);

protected:
    void Destroy();
    bool AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddEvent(ev_signal* signal_watcher, signal_callback pFunc, int iSignum);
    bool AddEvent(ev_timer* timer_watcher, timer_callback pFunc, ev_tstamp dTimeout);
    bool AddEvent(ev_idle* idle_watcher, idle_callback pFunc);
    bool AddEvent(ev_async* async_watcher, async_callback pFunc);
    bool AddEvent(ev_prepare* prepare_watcher, prepare_callback pFunc);
    bool AddEvent(ev_check* check_watcher, check_callback pFunc);
    bool RefreshEvent(ev_timer* timer_watcher, ev_tstamp dTimeout);
    bool DelEvent(ev_io* io_watcher);
    bool DelEvent(ev_timer* timer_watcher);
    int32 GetConnectionNum() const;
    void SetChannelStatus(std::shared_ptr<SocketChannel> pChannel, E_CHANNEL_STATUS eStatus);
    bool AddClientConnFrequencyTimeout(const char* pAddr, ev_tstamp dTimeout = 60.0);
    bool AcceptFdAndTransfer(int iFd, int iFamily = AF_INET, int iBonding = 0);
    bool AcceptServerConn(int iFd);
    bool PingChannel(std::shared_ptr<SocketChannel> pChannel);
    void CheckFailedNode();
    void EvBreak();
    PollingWatcher* MutablePollingWatcher();

private:
    char* m_pErrBuff;
    uint32 m_uiUpstreamConnectionPoolSize;
    Labor* m_pLabor;
    struct ev_loop* m_loop;
    time_t m_lLastCheckNodeTime;
    PollingWatcher* m_pPollingWatcher;
    std::shared_ptr<NetLogger> m_pLogger;
    std::unique_ptr<Nodes> m_pSessionNode;
    std::shared_ptr<SocketChannel> m_pLastActivityChannel;  // 最近一个发送或接收过数据的channel

    // Channel
    std::unordered_map<int32, std::shared_ptr<SocketChannel> > m_mapSocketChannel;

    /* named Channel */
    std::unordered_map<std::string, std::list<std::shared_ptr<SocketChannel> > > m_mapNamedSocketChannel;      ///< key为Identify，连接存在时，if(http连接)set.size()>=1;else set.size()==1;
    std::unordered_map<int32, std::string> m_mapChannelPingStepName;   // CODEC_TYPE as key 

    std::unordered_map<std::string, uint32> m_mapClientConnFrequency;   ///< 客户端连接频率

    friend class Manager;
    friend class Worker;
    friend class ActorBuilder;
    friend class LoadStress;
    friend class CodecFactory;
    friend class CmdFdTransfer;
    template<typename T> friend class IO;
};

template <typename ...Targs>
void Dispatcher::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

} /* namespace neb */

#endif /* SRC_IOS_DISPATCHER_HPP_ */
