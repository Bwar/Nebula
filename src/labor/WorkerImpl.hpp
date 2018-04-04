/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.hpp
 * @brief    工作者
 * @author   Bwar
 * @date:    2018年2月27日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_WORKERIMPL_HPP_
#define SRC_LABOR_WORKERIMPL_HPP_

#include <dlfcn.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <sstream>
#include "ev.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"

#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "pb/neb_sys.pb.h"
#include "util/CBuffer.hpp"
#include "labor/Labor.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/RedisChannel.hpp"
#include "codec/Codec.hpp"
#include "util/logger/NetLogger.hpp"

#include "actor/Actor.hpp"
#include "actor/ActorFactory.hpp"
#include "actor/cmd/CmdModel.hpp"
#include "actor/cmd/ModuleModel.hpp"
#include "actor/session/SessionModel.hpp"
#include "actor/step/StepModel.hpp"

namespace neb
{

class Worker;

class Actor;
class Cmd;
class Module;
class Session;
class Timer;
class Step;
class RedisStep;
class HttpStep;

class WorkerImpl
{
public:
    struct tagWorkerInfo
    {
        uint32 uiNodeId;                    ///< 节点ID
        int32 iManagerControlFd;            ///< 与Manager父进程通信fd（控制流）
        int32 iManagerDataFd;               ///< 与Manager父进程通信fd（数据流）
        int32 iWorkerIndex;
        int32 iWorkerPid;
        int32 iRecvNum;                     ///< 接收数据包（head+body）数量
        int32 iRecvByte;                    ///< 接收字节数（已到达应用层缓冲区）
        int32 iSendNum;                     ///< 发送数据包（head+body）数量（只到达应用层缓冲区，不一定都已发送出去）
        int32 iSendByte;                    ///< 发送字节数（已到达系统发送缓冲区，可认为已发送出去）
        int32 iMsgPermitNum;                ///< 客户端统计时间内允许发送消息数量
        ev_tstamp dMsgStatInterval;       ///< 客户端连接发送数据包统计时间间隔
        ev_tstamp dIoTimeout;             ///< IO（连接）超时配置
        ev_tstamp dStepTimeout;           ///< 步骤超时
        int iPortForServer;                 ///< Server间通信监听端口（用于生成当前Worker标识）
        std::string strHostForServer;       ///< 对其他Server服务的IP地址（用于生成当前Worker标识）
        std::string strNodeType;            ///< 节点类型
        std::string strWorkPath;            ///< 工作路径
        std::string strWorkerIdentify;      ///< 进程标识

        tagWorkerInfo()
            : uiNodeId(0), iManagerControlFd(0), iManagerDataFd(0), iWorkerIndex(0), iWorkerPid(0),
              iRecvNum(0), iRecvByte(0), iSendNum(0), iSendByte(0), iMsgPermitNum(60),
              dMsgStatInterval(60.0), dIoTimeout(480.0), dStepTimeout(3.0), iPortForServer(15000)
        {
        }
    };

    struct tagSo
    {
        void* pSoHandle;
        int iVersion;
        std::vector<int> vecCmd;
        std::vector<std::string> vecPath;
        tagSo();
        ~tagSo();
    };

    // callback
    static void TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void RedisConnectCallback(const redisAsyncContext *c, int status);
    static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
    static void RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata);

public:
    typedef std::unordered_map<std::string, std::pair<std::unordered_set<std::string>::iterator, std::unordered_set<std::string> > > T_MAP_NODE_TYPE_IDENTIFY;
public:
    WorkerImpl(Worker* pWorker, const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf);
    WorkerImpl(const WorkerImpl&) = delete;
    WorkerImpl& operator=(const WorkerImpl&) = delete;
    virtual ~WorkerImpl();

    void Terminated(struct ev_signal* watcher);
    bool CheckParent();
    bool OnIoRead(SocketChannel* pChannel);
    bool RecvDataAndHandle(SocketChannel* pChannel);
    bool FdTransfer();
    bool OnIoWrite(SocketChannel* pChannel);
    bool OnIoTimeout(SocketChannel* pChannel);
    bool OnStepTimeout(Step* pStep);
    bool OnSessionTimeout(Session* pSession);
    bool OnRedisConnected(const redisAsyncContext *c, int status);
    bool OnRedisDisconnected(const redisAsyncContext *c, int status);
    bool OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);

public:     // about worker
    virtual uint32 GetSequence() const
    {
        ++m_ulSequence;
        if (0 == m_ulSequence)
        {
            ++m_ulSequence;
        }
        return(m_ulSequence);
    }

    const tagWorkerInfo& GetWorkerInfo() const
    {
        return(m_stWorkerInfo);
    }

    const CJsonObject& GetCustomConf() const
    {
        return(m_oCustomConf);
    }

    virtual time_t GetNowTime() const;
    virtual bool ResetTimeout(Actor* pObject);

    template <typename ...Targs> void Logger(const std::string& strTraceId, int iLogLevel, Targs... args);
    template <typename ...Targs> Step* NewStep(Actor* pCreator, const std::string& strStepName, Targs... args);
    template <typename ...Targs> Session* NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args);
    template <typename ...Targs> Cmd* NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args);
    template <typename ...Targs> Module* NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args);

public:     // about channel
    virtual bool SendTo(const tagChannelContext& stCtx);
    virtual bool SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendPolling(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, unsigned int uiFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(RedisChannel* pRedisChannel, RedisStep* pRedisStep);
    virtual bool SendTo(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual bool AutoSend(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual bool Disconnect(const tagChannelContext& stCtx, bool bChannelNotice = true);
    virtual bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);
    virtual std::string GetClientAddr(const tagChannelContext& stCtx);
    virtual bool DiscardNamedChannel(const std::string& strIdentify);
    virtual bool SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType);
    bool AddIoTimeout(const tagChannelContext& stCtx);

public:     // about session
    virtual Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "neb::Session");
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "neb::Session");

public:     // Worker相关设置（由专用Cmd类调用这些方法完成Worker自身的初始化和更新）
    virtual bool SetProcessName(const CJsonObject& oJsonConf);
    virtual bool AddNamedSocketChannel(const std::string& strIdentify, const tagChannelContext& stCtx);
    virtual bool AddNamedSocketChannel(const std::string& strIdentify, SocketChannel* pChannel);
    virtual void DelNamedSocketChannel(const std::string& strIdentify);
    virtual bool SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify);
    virtual bool AddNamedRedisChannel(const std::string& strIdentify, redisAsyncContext* pCtx);
    virtual bool AddNamedRedisChannel(const std::string& strIdentify, RedisChannel* pChannel);
    virtual void DelNamedRedisChannel(const std::string& strIdentify);
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void AddInnerChannel(const tagChannelContext& stCtx);
    virtual void SetNodeId(uint32 uiNodeId) {m_stWorkerInfo.uiNodeId = uiNodeId;}
    virtual bool SetClientData(const tagChannelContext& stCtx, const std::string& strClientData);

protected:
    void Run();
    bool Init(CJsonObject& oJsonConf);
    bool InitLogger(const CJsonObject& oJsonConf);
    bool CreateEvents();
    void LoadSysCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(int iFd);
    bool AddIoWriteEvent(int iFd);
    bool RemoveIoWriteEvent(int iFd);
    bool AddIoReadEvent(SocketChannel* pChannel);
    bool AddIoWriteEvent(SocketChannel* pChannel);
    bool RemoveIoWriteEvent(SocketChannel* pChannel);
    bool AddIoTimeout(SocketChannel* pChannel, ev_tstamp dTimeout = 1.0);
    SocketChannel* CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType);
    bool DiscardSocketChannel(SocketChannel* pChannel, bool bChannelNotice = true);
    void Remove(Step* pStep);
    void Remove(Session* pSession);
    void ChannelNotice(const tagChannelContext& stCtx, const std::string& strIdentify, const std::string& strClientData);

    /**
     * @brief 收到完整数据包后处理
     * @note
     * <pre>
     * 框架层接收并解析到完整的数据包后调用此函数做数据处理。数据处理可能有多重不同情况出现：
     * 1. 处理成功，仍需继续解析数据包；
     * 2. 处理成功，但无需继续解析数据包；
     * 3. 处理失败，仍需继续解析数据包；
     * 4. 处理失败，但无需继续解析数据包。
     * 是否需要退出解析数据包循环体取决于Dispose()的返回值。返回true就应继续解析数据包，返回
     * false则应退出解析数据包循环体。处理过程或处理完成后，如需回复对端，则直接使用pSendBuff
     * 回复数据即可。
     * </pre>
     * @param[in] stMsgShell 数据包来源消息通道
     * @param[in] oMsgHead 接收的数据包头
     * @param[in] oMsgBody 接收的数据包体
     * @return 是否正常处理
     */
    bool Handle(SocketChannel* pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    /**
     * @brief 收到完整的hhtp包后处理
     * @param stMsgShell 数据包来源消息通道
     * @param oHttpMsg 接收的HTTP包
     * @return 是否正常处理
     */
    bool Handle(SocketChannel* pChannel, const HttpMsg& oHttpMsg);

    void LoadCmd(CJsonObject& oCmdConf);
    tagSo* LoadSo(const std::string& strSoPath, int iVersion);

private:
    mutable uint32 m_ulSequence = 0;
    char* m_pErrBuff;
    Worker* m_pWorker;
    std::shared_ptr<NetLogger> m_pLogger = nullptr;

    tagWorkerInfo m_stWorkerInfo;
    CJsonObject m_oCustomConf;    ///< 自定义配置

    struct ev_loop* m_loop;
    Cmd* m_pCmdConnect;

    // Cmd and Module
    std::unordered_map<int32, Cmd*> m_mapCmd;
    std::unordered_map<std::string, Module*> m_mapModule;
    std::unordered_map<std::string, tagSo*> m_mapLoadedSo;

    // Step and Session
    std::unordered_map<uint32, Step* > m_mapCallbackStep;
    std::unordered_map<std::string, std::unordered_map<std::string, Session*> > m_mapCallbackSession;

    // Channel
    std::unordered_map<int, SocketChannel*> m_mapSocketChannel;            ///< 通信通道
    std::unordered_map<redisAsyncContext*, RedisChannel*>  m_mapRedisChannel;
    std::unordered_map<int, uint32> m_mapInnerFd;              ///< 服务端之间连接的文件描述符（用于区分连接是服务内部还是外部客户端接入）
    std::unordered_map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）

    /* named Channel */
    std::unordered_map<std::string, std::list<SocketChannel*> > m_mapNamedSocketChannel;      ///< key为Identify，连接存在时，if(http连接)list.size()>=1;else list.size()==1;
    std::unordered_map<std::string, std::list<RedisChannel*> > m_mapNamedRedisChannel;        ///< key为identify，list.size()==1
    std::unordered_map<std::string, std::string> m_mapIdentifyNodeType;    // key为Identify，value为node_type
    T_MAP_NODE_TYPE_IDENTIFY m_mapNodeIdentify;
};



template <typename ...Targs>
void WorkerImpl::Logger(const std::string& strTraceId, int iLogLevel, Targs... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, std::forward<Targs>(args)...);
}

template <typename ...Targs>
Step* WorkerImpl::NewStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    Step* pStep = dynamic_cast<Step*>(ActorFactory<Targs...>::Instance()->Create(strStepName, std::forward<Targs>(args)...));
    if (nullptr == pStep)
    {
        return(nullptr);
    }
    StepModel* pStepAlias = (StepModel*)pStep;
    pStepAlias->m_dTimeout = (0 == pStepAlias->m_dTimeout) ? m_stWorkerInfo.dStepTimeout : pStepAlias->m_dTimeout;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStepAlias, pStepAlias->m_dTimeout);

    pStepAlias->SetWorker(m_pWorker);
    pStepAlias->SetActiveTime(ev_now(m_loop));
    if (nullptr != pCreator)
    {
        switch(pCreator->m_eActorType)
        {
            case Actor::ACT_PB_STEP:
            case Actor::ACT_HTTP_STEP:
            case Actor::ACT_REDIS_STEP:
            case Actor::ACT_CMD:
            case Actor::ACT_MODULE:
                pStepAlias->m_strTraceId = pCreator->m_strTraceId;
                break;
            case Actor::ACT_SESSION:
            case Actor::ACT_TIMER:
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                pStepAlias->m_strTraceId = std::move(oss.str());
            }
                break;
            default:
                ;
        }
    }
    ev_timer* timer_watcher = pStepAlias->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pStep;
        pStepAlias = pStep = nullptr;
        return(nullptr);
    }

    for (auto iter = pStepAlias->m_setNextStepSeq.begin(); iter != pStepAlias->m_setNextStepSeq.end(); ++iter)
    {
        auto callback_iter = m_mapCallbackStep.find(*iter);
        if (callback_iter != m_mapCallbackStep.end())
        {
            ((StepModel*)callback_iter->second)->m_setPreStepSeq.insert(pStepAlias->GetSequence());
        }
    }

    auto ret = m_mapCallbackStep.insert(std::make_pair(pStepAlias->GetSequence(), pStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pStepAlias->m_dTimeout)
        {
            ev_timer_init (timer_watcher, StepTimeoutCallback, pStepAlias->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStepAlias->GetSequence(), pStepAlias->GetActiveTime(), pStepAlias->GetTimeout());
        return(pStep);
    }
    else
    {
        delete pStep;
        pStepAlias = pStep = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Session* WorkerImpl::NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    Session* pSession = dynamic_cast<Session*>(ActorFactory<Targs...>::Instance()->Create(strSessionName, std::forward<Targs>(args)...));
    if (nullptr == pSession)
    {
        return(nullptr);
    }
    SessionModel* pSessionAlias = (SessionModel*)pSession;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pSession, pSessionAlias->m_dTimeout);

    pSessionAlias->SetWorker(m_pWorker);
    pSessionAlias->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pSessionAlias->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pSession;
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }

    std::pair<std::unordered_map<std::string, Session*>::iterator, bool> ret;
    auto session_name_iter = m_mapCallbackSession.find(pSessionAlias->GetSessionClass());
    if (session_name_iter == m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::make_pair(pSessionAlias->GetSessionId(), pSession));
        m_mapCallbackSession.insert(std::make_pair(pSessionAlias->GetSessionClass(), mapSession));
    }
    else
    {
        ret = session_name_iter->second.insert(std::make_pair(pSessionAlias->GetSessionId(), pSession));
    }
    if (ret.second)
    {
        ev_timer_init (timer_watcher, StepTimeoutCallback, pSessionAlias->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSessionAlias->GetSequence(), pSessionAlias->GetActiveTime(), pSessionAlias->GetTimeout());
        return(pSession);
    }
    else
    {
        delete pSession;
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Cmd* WorkerImpl::NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    Cmd* pCmd = dynamic_cast<Cmd*>(ActorFactory<Targs...>::Instance()->Create(strCmdName, std::forward<Targs>(args)...));
    if (nullptr == pCmd)
    {
        return(nullptr);
    }
    CmdModel* pCmdAlias = (CmdModel*)pCmd;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Cmd* 0x%X)", __FUNCTION__, pCmd);

    pCmdAlias->SetWorker(m_pWorker);
    pCmdAlias->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapCmd.insert(std::make_pair(pCmdAlias->GetCmd(), pCmd));
    if (ret.second)
    {
        return(pCmd);
    }
    else
    {
        delete pCmd;
        pCmdAlias = pCmd = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Module* WorkerImpl::NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    Module* pModule = dynamic_cast<Module*>(ActorFactory<Targs...>::Instance()->Create(strModuleName, std::forward<Targs>(args)...));
    if (nullptr == pModule)
    {
        return(nullptr);
    }
    ModuleModel* pModuleAlias = (ModuleModel*)pModule;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Module* 0x%X)", __FUNCTION__, pModule);

    pModuleAlias->SetWorker(m_pWorker);
    pModuleAlias->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapModule.insert(std::make_pair(pModuleAlias->GetModulePath(), pModule));
    if (ret.second)
    {
        return(pModule);
    }
    else
    {
        delete pModule;
        pModuleAlias = pModule = nullptr;
        return(nullptr);
    }
}

} /* namespace neb */

#endif /* SRC_LABOR_WORKERIMPL_HPP_ */
