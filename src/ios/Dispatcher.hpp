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
#include <unordered_set>
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
struct tagClientConnWatcherData;
template<typename T> class IO;

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
    bool SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    std::shared_ptr<SocketChannel> StressSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    // SendTo() for unix domain socket
    bool SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendTo(int iFd, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    std::shared_ptr<SocketChannel> GetLastActivityChannel();
    bool Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);
    bool DiscardNamedChannel(const std::string& strIdentify);

public:
    void SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify);
    bool AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel);
    void DelNamedSocketChannel(const std::string& strIdentify);
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void CircuitBreak(const std::string& strIdentify);
    void SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData);
    bool IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType);
    bool GetAuth(const std::string& strNodeType, std::string& strAuth, std::string& strPassword);
    void SetAuth(const std::string& strNodeType, const std::string& strAuth, const std::string& strPassword);

    time_t GetNowTime() const
    {
        return((time_t)ev_now(m_loop));
    }
    long GetNowTimeMs() const
    {
        return((long)ev_now(m_loop) * 1000);
    }
    std::shared_ptr<NetLogger> GetLogger() const
    {
        return(m_pLogger);
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
    friend class CodecFactory;
    template<typename T> friend class IO;
};

template <typename ...Targs>
void Dispatcher::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

} /* namespace neb */

#endif /* SRC_IOS_DISPATCHER_HPP_ */
