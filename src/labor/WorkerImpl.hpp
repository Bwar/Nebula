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

#ifdef __cplusplus
extern "C" {
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "ev.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <queue>
#include <sstream>
#include <fstream>

#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "pb/neb_sys.pb.h"
#include "util/CBuffer.hpp"
#include "labor/Labor.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/RedisChannel.hpp"
#include "codec/Codec.hpp"
#include "logger/NetLogger.hpp"

#include "actor/ActorFactory.hpp"

namespace neb
{

class Worker;

class Actor;
class Cmd;
class Module;
class Session;
class Timer;
class Context;
class Step;
class RedisStep;
class HttpStep;
class Model;
class Chain;

class SessionNode;
class SessionLogger;

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
        int32 iConnectionNum;
        int32 iClientNum;
        int32 iRecvNum;                     ///< 接收数据包（head+body）数量
        int32 iRecvByte;                    ///< 接收字节数（已到达应用层缓冲区）
        int32 iSendNum;                     ///< 发送数据包（head+body）数量（只到达应用层缓冲区，不一定都已发送出去）
        int32 iSendByte;                    ///< 发送字节数（已到达系统发送缓冲区，可认为已发送出去）
        int32 iMsgPermitNum;                ///< 客户端统计时间内允许发送消息数量
        bool bIsAccess;                     ///< 是否接入Server
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
              iConnectionNum(0), iClientNum(0),
              iRecvNum(0), iRecvByte(0), iSendNum(0), iSendByte(0), iMsgPermitNum(60), bIsAccess(false),
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
        tagSo(){};
        ~tagSo(){};
    };

    // callback
    static void TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void ChainTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void RedisConnectCallback(const redisAsyncContext *c, int status);
    static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
    static void RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata);

public:
    WorkerImpl(Worker* pWorker, const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf);
    WorkerImpl(const WorkerImpl&) = delete;
    WorkerImpl& operator=(const WorkerImpl&) = delete;
    virtual ~WorkerImpl();

    void Terminated(struct ev_signal* watcher);
    bool CheckParent();
    bool OnIoRead(std::shared_ptr<SocketChannel> pChannel);
    bool RecvDataAndHandle(std::shared_ptr<SocketChannel> pChannel);
    bool FdTransfer();
    bool OnIoWrite(std::shared_ptr<SocketChannel> pChannel);
    bool OnIoTimeout(std::shared_ptr<SocketChannel> pChannel);
    bool OnStepTimeout(std::shared_ptr<Step> pStep);
    bool OnSessionTimeout(std::shared_ptr<Session> pSession);
    bool OnChainTimeout(std::shared_ptr<Chain> pChain);
    bool OnRedisConnected(const redisAsyncContext *c, int status);
    bool OnRedisDisconnected(const redisAsyncContext *c, int status);
    bool OnRedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);

    void Run();

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
    virtual bool ResetTimeout(std::shared_ptr<Actor> pSharedActor);

    template <typename ...Targs>
        void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);
    template <typename ...Targs>
        void Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Actor> MakeSharedActor(Actor* pCreator, const std::string& strActorName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Cmd> MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Module> MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Step> MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Session> MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Context> MakeSharedContext(Actor* pCreator, const std::string& strContextName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Model> MakeSharedModel(Actor* pCreator, const std::string& strModelName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Chain> MakeSharedChain(Actor* pCreator, const std::string& strChainName, Targs&&... args);

    std::shared_ptr<Actor> InitializeSharedActor(Actor* pCreator, std::shared_ptr<Actor> pSharedActor, const std::string& strActorName);
    bool TransformToSharedCmd(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedModule(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedStep(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedSession(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedModel(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedChain(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);

public:     // about channel
    virtual bool AddNetLogMsg(const MsgBody& oMsgBody);

    // SendTo() for nebula socket
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel);
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool AutoSend(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    // SendTo() for http
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);

    // SendTo() for redis
    virtual bool SendTo(std::shared_ptr<RedisChannel> pRedisChannel, Actor* pSender);
    virtual bool SendTo(const std::string& strIdentify, Actor* pSender);
    virtual bool SendTo(const std::string& strHost, int iPort, Actor* pSender);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, std::shared_ptr<RedisStep> pRedisStep);

    virtual bool Disconnect(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    virtual bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);
    virtual bool DiscardNamedChannel(const std::string& strIdentify);
    virtual bool SwitchCodec(std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eCodecType);

public:     
    // about session
    virtual std::shared_ptr<Session> GetSession(uint32 uiSessionId);
    virtual std::shared_ptr<Session> GetSession(const std::string& strSessionId);

    // about step
    virtual bool ExecStep(uint32 uiStepSeq, int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);

    // about model
    virtual std::shared_ptr<Model> GetModel(const std::string& strModelName);

public:     // Worker相关设置（由专用Cmd类调用这些方法完成Worker自身的初始化和更新）
    virtual bool SetProcessName(const CJsonObject& oJsonConf);
    virtual bool AddNamedSocketChannel(const std::string& strIdentify, std::shared_ptr<SocketChannel> pChannel);
    virtual void DelNamedSocketChannel(const std::string& strIdentify);
    virtual void SetChannelIdentify(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify);
    virtual bool AddNamedRedisChannel(const std::string& strIdentify, std::shared_ptr<RedisChannel> pChannel);
    virtual void DelNamedRedisChannel(const std::string& strIdentify);
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void SetNodeId(uint32 uiNodeId) {m_stWorkerInfo.uiNodeId = uiNodeId;}
    virtual void SetClientData(std::shared_ptr<SocketChannel> pChannel, const std::string& strClientData);

    bool AddIoTimeout(std::shared_ptr<SocketChannel> pChannel, ev_tstamp dTimeout = 1.0);
    void AddAssemblyLine(std::shared_ptr<Session> pSession);

    bool SetWorkerConf(const CJsonObject& oJsonConf);
    const CJsonObject& GetWorkerConf() const
    {
        return(m_oWorkerConf);
    }
    bool SetCustomConf(const CJsonObject& oJsonConf);
    bool ReloadCmdConf();

protected:
    bool Init(CJsonObject& oJsonConf);
    bool InitLogger(const CJsonObject& oJsonConf);
    bool CreateEvents();
    void LoadSysCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(std::shared_ptr<SocketChannel> pChannel);
    bool AddIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    bool RemoveIoWriteEvent(std::shared_ptr<SocketChannel> pChannel);
    std::shared_ptr<SocketChannel> CreateSocketChannel(int iFd, E_CODEC_TYPE eCodecType, bool bIsClient = false, bool bWithSsl = false);
    bool DiscardSocketChannel(std::shared_ptr<SocketChannel> pChannel, bool bChannelNotice = true);
    void RemoveStep(std::shared_ptr<Step> pStep);
    void RemoveSession(std::shared_ptr<Session> pSession);
    void RemoveChain(uint32 uiChainId);
    void ChannelNotice(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const std::string& strClientData);

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
     * @param[in] pChannel 数据包来源消息通道
     * @param[in] oMsgHead 接收的数据包头
     * @param[in] oMsgBody 接收的数据包体
     * @return 是否正常处理
     */
    bool Handle(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    /**
     * @brief 收到完整的hhtp包后处理
     * @param pChannel 数据包来源消息通道
     * @param oHttpMsg 接收的HTTP包
     * @return 是否正常处理
     */
    bool Handle(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);

    void ExecAssemblyLine(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    void BootLoadCmd(CJsonObject& oCmdConf);
    void DynamicLoad(CJsonObject& oSoConf);
    tagSo* LoadSo(const std::string& strSoPath, int iVersion);
    void LoadDynamicSymbol(CJsonObject& oOneSoConf);
    void UnloadDynamicSymbol(CJsonObject& oOneSoConf);

private:
    mutable uint32 m_ulSequence = 0;
    char* m_pErrBuff;
    Worker* m_pWorker;
    std::shared_ptr<NetLogger> m_pLogger = nullptr;

    tagWorkerInfo m_stWorkerInfo;
    CJsonObject m_oWorkerConf;
    CJsonObject m_oCustomConf;    ///< 自定义配置

    struct ev_loop* m_loop;
    std::shared_ptr<SocketChannel> m_pManagerControlChannel;        // std::unique_ptr<SocketChannel> is perfect
    std::shared_ptr<SocketChannel> m_pManagerDataChannel;
    std::unique_ptr<SessionNode> m_pSessionNode;
    std::shared_ptr<SessionLogger> m_pSessionLogger;

    // dynamic load，use for load and unload.
    std::unordered_map<std::string, tagSo*> m_mapLoadedSo;
    std::unordered_map<std::string, std::unordered_set<int32> > m_mapLoadedCmd;             //key为CmdClassName，value为iCmd集合
    std::unordered_map<std::string, std::unordered_set<std::string> > m_mapLoadedModule;    //key为ModuleClassName，value为strModulePath集合
    std::unordered_map<std::string, std::unordered_set<uint32> > m_mapLoadedStep;           //key为StepClassName，value为uiSeq集合
    std::unordered_map<std::string, std::unordered_set<std::string> > m_mapLoadedSession;   //key为SessionClassName，value为strSessionId集合

    // Cmd and Module
    std::unordered_map<int32, std::shared_ptr<Cmd> > m_mapCmd;
    std::unordered_map<std::string, std::shared_ptr<Module> > m_mapModule;

    // Chain and Model
    std::unordered_map<std::string, std::queue<std::vector<std::string> > > m_mapChainConf; //key为Chain的配置名(ChainFlag)，value为由Model类名和Step类名构成的ChainBlock链
    std::unordered_map<uint32, std::shared_ptr<Chain> > m_mapChain;                         //key为Chain的Sequence，称为ChainId
    std::unordered_map<std::string, std::shared_ptr<Model> > m_mapModel;                  //key为Model类名

    // Step and Session
    std::unordered_map<uint32, std::shared_ptr<Step> > m_mapCallbackStep;
    std::unordered_map<std::string, std::shared_ptr<Session> > m_mapCallbackSession;
    std::unordered_set<std::shared_ptr<Session> > m_setAssemblyLine;   ///< 资源就绪后执行队列

    // Channel
    std::unordered_map<int32, std::shared_ptr<SocketChannel> > m_mapSocketChannel;
    std::unordered_map<redisAsyncContext*, std::shared_ptr<RedisChannel> >  m_mapRedisChannel;

    /* named Channel */
    std::unordered_map<std::string, std::list<std::shared_ptr<SocketChannel> > > m_mapNamedSocketChannel;      ///< key为Identify，连接存在时，if(http连接)list.size()>=1;else list.size()==1;
    std::unordered_map<std::string, std::list<std::shared_ptr<RedisChannel> > > m_mapNamedRedisChannel;        ///< key为identify，list.size()==1
};


} /* namespace neb */


#include "WorkerImpl.inl"

#endif /* SRC_LABOR_WORKERIMPL_HPP_ */
