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
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include "ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/socketappender.h"
#include "log4cplus/loggingmacros.h"

#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "pb/msg.pb.h"
#include "pb/neb_sys.pb.h"
#include "Error.hpp"
#include "Definition.hpp"
#include "Attribute.hpp"
#include "labor/Labor.hpp"
#include "Worker.hpp"

#include "cmd/Cmd.hpp"

namespace neb
{

class Worker;

class Manager: public Labor
{
public:
    Manager(const std::string& strConfFile);
    virtual ~Manager();

    static void SignalCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void ClientConnFrequencyTimeoutCallback(struct ev_loop* loop, struct ev_timer* watcher, int revents);
    bool ManagerTerminated(struct ev_signal* watcher);
    bool ChildTerminated(struct ev_signal* watcher);
    bool IoRead(Channel* pChannel);
    bool IoWrite(Channel* pChannel);
    bool IoError(Channel* pChannel);
    bool IoTimeout(Channel* pChannel);
    bool ClientConnFrequencyTimeout(tagClientConnWatcherData* pData, struct ev_timer* watcher);

    void Run();

public:     // Manager相关设置（由专用Cmd类调用这些方法完成Manager自身的初始化和更新）
    bool InitLogger(const loss::CJsonObject& oJsonConf);
    virtual bool SetProcessName(const loss::CJsonObject& oJsonConf);
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool SendTo(const tagChannelContext& stCtx);
    virtual bool SendTo(const tagChannelContext& stCtx, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual bool SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerChannel(const tagChannelContext& stCtx){};

    void SetWorkerLoad(int iPid, loss::CJsonObject& oJsonLoad);
    void AddWorkerLoad(int iPid, int iLoad = 1);
    const std::map<int, tagWorkerAttr>& GetWorkerAttr() const;

protected:
    bool GetConf();
    bool Init();
    void Destroy();
    void CreateWorker();
    bool CreateEvents();
    bool RegisterToKeeper();
    bool RestartWorker(int iDeathPid);
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(Channel* pChannel);
    bool AddIoWriteEvent(Channel* pChannel);
    bool RemoveIoWriteEvent(Channel* pChannel);
    bool DelEvents(ev_io* io_watcher);
    bool DelEvents(ev_timer* timer_watcher);
    bool AddIoTimeout(Channel* pChannel, ev_tstamp dTimeout = 1.0);
    bool AddClientConnFrequencyTimeout(in_addr_t iAddr, ev_tstamp dTimeout = 60.0);
    Channel* CreateChannel(int iFd, E_CODEC_TYPE eCodecType);
    bool DestroyConnect(std::map<int, Channel*>::iterator iter);
    bool DestroyConnect(Channel* pChannel);
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool FdTransfer(int iFd);
    bool AcceptServerConn(int iFd);
    bool DataRecvAndHandle(Channel* pChannel);
    bool CheckWorker();
    void RefreshServer();
    bool ReportToKeeper();  // 向管理中心上报负载信息
    bool SendToWorker(const MsgHead& oMsgHead, const MsgBody& oMsgBody);    // 向Worker发送数据
    bool HandleDataFromWorker(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    bool HandleDataAndTransferFd(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    bool HandleDataFromKeeper(Channel* pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);

    uint32 GetSequence()
    {
        return(m_ulSequence++);
    }

    log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

private:
    loss::CJsonObject m_oLastConf;          ///< 上次加载的配置
    loss::CJsonObject m_oCurrentConf;       ///< 当前加载的配置

    uint32 m_ulSequence;
    char m_szErrBuff[256];
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO超时配置

    std::string m_strConfFile;              ///< 配置文件
    std::string m_strWorkPath;              ///< 工作路径
    std::string m_strNodeType;              ///< 节点类型
    uint32 m_uiNodeId;                      ///< 节点ID（由center分配）
    std::string m_strHostForServer;         ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
    std::string m_strHostForClient;         ///< 对Client服务的IP地址，对应 m_iC2SListenFd
    std::string m_strGateway;               ///< 对Client服务的真实IP地址（此ip转发给m_strHostForClient）
    int32 m_iPortForServer;                 ///< Server间通信监听端口，对应 m_iS2SListenFd
    int32 m_iPortForClient;                 ///< 对Client通信监听端口，对应 m_iC2SListenFd
    int32 m_iGatewayPort;                   ///< 对Client服务的真实端口
    uint32 m_uiWorkerNum;                   ///< Worker子进程数量
    E_CODEC_TYPE m_eCodec;            ///< 接入端编解码器
    ev_tstamp m_dAddrStatInterval;          ///< IP地址数据统计时间间隔
    int32  m_iAddrPermitNum;                ///< IP地址统计时间内允许连接次数
    int m_iLogLevel;
    int m_iWorkerBeat;                      ///< worker进程心跳，若大于此心跳未收到worker进程上报，则重启worker进程
    int m_iRefreshCalc;                     ///< Server刷新计数

    int m_iS2SListenFd;                     ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
    int m_iC2SListenFd;                     ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
    struct ev_loop* m_loop;
    ev_timer* m_pPeriodicTaskWatcher;              ///< 周期任务定时器

    std::map<int, tagWorkerAttr> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::map<int, int> m_mapWorkerRestartNum;       ///< 进程被重启次数，key为WorkerIdx
    std::map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号
    std::map<std::string, tagChannelContext> m_mapKeeperCtx; ///< 到Keeper服务器的连接

    std::map<int, Channel*> m_mapChannel;                   ///< 通信通道
    std::map<uint32, int> m_mapSeq2WorkerIndex;             ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）
    std::map<in_addr_t, uint32> m_mapClientConnFrequency;   ///< 客户端连接频率
    std::map<int32, Cmd*> m_mapCmd;

    std::vector<int> m_vecFreeWorkerIdx;            ///< 空闲进程编号
};

} /* namespace neb */

#endif /* SRC_LABOR_CHIEF_MANAGER_HPP_ */
