/*******************************************************************************
 * Project:  Nebula
 * @file     Manager.hpp
 * @brief    管理者
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_CHIEF_MANAGER_HPP_
#define SRC_LABOR_CHIEF_MANAGER_HPP_

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

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <list>

#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "pb/msg.pb.h"
#include "pb/neb_sys.pb.h"
#include "Error.hpp"
#include "Definition.hpp"
#include "util/logger/NetLogger.hpp"
#include "Labor.hpp"
#include "channel/Channel.hpp"
#include "actor/cmd/Cmd.hpp"
#include "actor/session/sys_session/SessionNode.hpp"


namespace neb
{

class Worker;
class Actor;

class Manager: public Labor
{
public:
    struct tagClientConnWatcherData
    {
        in_addr_t iAddr;
        Labor* pLabor;     // 不在结构体析构时回收

        tagClientConnWatcherData() : iAddr(0), pLabor(NULL)
        {
        }
    };

    struct tagManagerInfo
    {
        E_CODEC_TYPE eCodec;                  ///< 接入端编解码器
        uint32 uiNodeId;                      ///< 节点ID（由center分配）
        int32 iPortForServer;                 ///< Server间通信监听端口，对应 iS2SListenFd
        int32 iPortForClient;                 ///< 对Client通信监听端口，对应 iC2SListenFd
        int32 iGatewayPort;                   ///< 对Client服务的真实端口
        uint32 uiWorkerNum;                   ///< Worker子进程数量
        int32 iAddrPermitNum;                 ///< IP地址统计时间内允许连接次数
        int iWorkerBeat;                      ///< worker进程心跳，若大于此心跳未收到worker进程上报，则重启worker进程
        int iS2SListenFd;                     ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
        int iC2SListenFd;                     ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
        ev_tstamp dIoTimeout;                 ///< IO超时配置
        ev_tstamp dAddrStatInterval;          ///< IP地址数据统计时间间隔
        std::string strWorkPath;              ///< 工作路径
        std::string strConfFile;              ///< 配置文件
        std::string strNodeType;              ///< 节点类型
        std::string strHostForServer;         ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
        std::string strHostForClient;         ///< 对Client服务的IP地址，对应 m_iC2SListenFd
        std::string strGateway;               ///< 对Client服务的真实IP地址（此ip转发给m_strHostForClient）
        std::string strNodeIdentify;

        tagManagerInfo()
            : eCodec(CODEC_HTTP), uiNodeId(0), iPortForServer(0), iPortForClient(0), iGatewayPort(0),
              uiWorkerNum(0), iAddrPermitNum(0),
              iWorkerBeat(7), iS2SListenFd(-1), iC2SListenFd(-1), dIoTimeout(300.0), dAddrStatInterval(60.0)
        {
        }
    };

    /**
     * @brief 工作进程属性
     */
    struct tagWorkerAttr
    {
        int iWorkerIndex;                   ///< 工作进程序号
        int iControlFd;                     ///< 与Manager进程通信的文件描述符（控制流）
        int iDataFd;                        ///< 与Manager进程通信的文件描述符（数据流）
        int32 iLoad;                        ///< 负载
        int32 iConnect;                     ///< 连接数量
        int32 iRecvNum;                     ///< 接收数据包数量
        int32 iRecvByte;                    ///< 接收字节数
        int32 iSendNum;                     ///< 发送数据包数量
        int32 iSendByte;                    ///< 发送字节数
        int32 iClientNum;                   ///< 客户端数量
        ev_tstamp dBeatTime;                ///< 心跳时间

        tagWorkerAttr()
        {
            iWorkerIndex = 0;
            iControlFd = -1;
            iDataFd = -1;
            iLoad = 0;
            iConnect = 0;
            iRecvNum = 0;
            iRecvByte = 0;
            iSendNum = 0;
            iSendByte = 0;
            iClientNum = 0;
            dBeatTime = 0.0;
        }

        tagWorkerAttr(const tagWorkerAttr& stAttr)
        {
            iWorkerIndex = stAttr.iWorkerIndex;
            iControlFd = stAttr.iControlFd;
            iDataFd = stAttr.iDataFd;
            iLoad = stAttr.iLoad;
            iConnect = stAttr.iConnect;
            iRecvNum = stAttr.iRecvNum;
            iRecvByte = stAttr.iRecvByte;
            iSendNum = stAttr.iSendNum;
            iSendByte = stAttr.iSendByte;
            iClientNum = stAttr.iClientNum;
            dBeatTime = stAttr.dBeatTime;
        }

        tagWorkerAttr& operator=(const tagWorkerAttr& stAttr)
        {
            iWorkerIndex = stAttr.iWorkerIndex;
            iControlFd = stAttr.iControlFd;
            iDataFd = stAttr.iDataFd;
            iLoad = stAttr.iLoad;
            iConnect = stAttr.iConnect;
            iRecvNum = stAttr.iRecvNum;
            iRecvByte = stAttr.iRecvByte;
            iSendNum = stAttr.iSendNum;
            iSendByte = stAttr.iSendByte;
            iClientNum = stAttr.iClientNum;
            dBeatTime = stAttr.dBeatTime;
            return(*this);
        }
    };

public:
    Manager(const std::string& strConfFile);
    virtual ~Manager();

    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    bool OnManagerTerminated(struct ev_signal* watcher);
    bool OnChildTerminated(struct ev_signal* watcher);
    bool OnIoRead(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoWrite(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoError(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoTimeout(std::shared_ptr<SocketChannel> pChannel);
    bool OnClientConnFrequencyTimeout(tagClientConnWatcherData* pData, ev_timer* watcher);

    void Run();

    template <typename ...Targs>
        void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);

public:
    bool InitLogger(const CJsonObject& oJsonConf);
    virtual bool SetProcessName(const CJsonObject& oJsonConf);
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel);
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendPolling(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual void SetNodeId(uint32 uiNodeId) {m_stManagerInfo.uiNodeId = uiNodeId;}
    uint32 GetSequence() const
    {
        return(m_uiSequence++);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_stManagerInfo.uiNodeId);
    }

    virtual time_t GetNowTime() const
    {
        return((time_t)ev_now(m_loop));
    }

    virtual const std::string& GetNodeIdentify() const
    {
        return(m_stManagerInfo.strNodeIdentify);
    }

    virtual void AddInnerChannel(std::shared_ptr<SocketChannel> pChannel){};

    void SetWorkerLoad(int iPid, CJsonObject& oJsonLoad);
    void AddWorkerLoad(int iPid, int iLoad = 1);

protected:
    bool GetConf();
    bool Init();
    void Destroy();

    bool CreateEvents();
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool DelEvents(ev_io* io_watcher);
    bool DelEvents(ev_timer* timer_watcher);

    void CreateWorker();
    bool CheckWorker();
    bool RestartWorker(int iDeathPid);
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);    // 向Worker发送数据
    void RefreshServer();

    bool RegisterToBeacon();
    bool ReportToBeacon();

    bool AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel);
    void DelNamedSocketChannel(const std::string& strIdentify);
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    bool AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout = 1.0);
    bool AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout = 60.0);
    std::shared_ptr<SocketChannel> CreateChannel(int iFd, E_CODEC_TYPE eCodecType);
    bool DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel);
    bool FdTransfer(int iFd);
    bool AcceptServerConn(int iFd);
    bool DataRecvAndHandle(std::shared_ptr<SocketChannel> pChannel);
    bool OnWorkerData(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    bool OnDataAndTransferFd(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    bool OnBeaconData(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    bool OnNodeNotify(const MsgBody& oMsgBody);

private:
    mutable uint32 m_uiSequence;
    CJsonObject m_oLastConf;          ///< 上次加载的配置
    CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    char m_szErrBuff[256];
    std::shared_ptr<NetLogger> m_pLogger = nullptr;
    struct ev_loop* m_loop;
    ev_timer* m_pPeriodicTaskWatcher;              ///< 周期任务定时器

    tagManagerInfo m_stManagerInfo;
    std::unique_ptr<SessionNode> m_pSessionNode = nullptr;

    std::unordered_map<int, tagWorkerAttr> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::unordered_map<int, int> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::unordered_map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号

    std::unordered_map<std::string, std::shared_ptr<SocketChannel> > m_mapNamedSocketChannel;
    std::unordered_map<int, std::shared_ptr<SocketChannel> > m_mapSocketChannel;                   ///< 通信通道
    std::unordered_map<in_addr_t, uint32> m_mapClientConnFrequency;   ///< 客户端连接频率

    std::vector<int> m_vecFreeWorkerIdx;            ///< 空闲进程编号
};

template <typename ...Targs>
void Manager::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

} /* namespace neb */

#endif /* SRC_LABOR_CHIEF_MANAGER_HPP_ */
