/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.hpp
 * @brief    工作者
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_CHIEF_WORKER_HPP_
#define SRC_LABOR_CHIEF_WORKER_HPP_

#include <map>
#include <list>
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
#include "ev.h"
#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/socketappender.h"
#include "log4cplus/loggingmacros.h"
#include "hiredis/hiredis.h"
#include "hiredis/adapters/libev.h"

#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "pb/neb_sys.pb.h"
#include "labor/Labor.hpp"
#include "Attribute.hpp"
#include "channel/Channel.hpp"
#include "object/Object.hpp"
#include "object/cmd/Cmd.hpp"
#include "object/cmd/Module.hpp"
#include "object/session/Session.hpp"
#include "object/step/Step.hpp"
#include "object/step/PbStep.hpp"
#include "object/step/HttpStep.hpp"
#include "object/step/RedisStep.hpp"

namespace neb
{

class Worker: public Labor
{
public:
public:
    typedef std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > > T_MAP_NODE_TYPE_IDENTIFY;
public:
    Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf);
    virtual ~Worker();

    void Run();

    static void TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents);
    static void IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents);
    static void IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents);
    static void IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents);  // 周期任务回调，用于替换IdleCallback
    static void StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void RedisConnectCallback(const redisAsyncContext *c, int status);
    static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
    static void RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata);
    void Terminated(struct ev_signal* watcher);
    bool CheckParent();
    bool IoRead(Channel* pChannel);
    bool RecvDataAndHandle(Channel* pChannel);
    bool FdTransfer();
    bool IoWrite(Channel* pChannel);
    bool IoTimeout(Channel* pChannel);
    bool StepTimeout(Step* pStep);
    bool SessionTimeout(Session* pSession);
    bool RedisConnect(const redisAsyncContext *c, int status);
    bool RedisDisconnect(const redisAsyncContext *c, int status);
    bool RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata);

public:     // Cmd类和Step类只需关注这些方法
    virtual uint32 GetSequence()
    {
        ++m_ulSequence;
        if (m_ulSequence == 0)      // Server长期运行，sequence达到最大正整数又回到0
        {
            ++m_ulSequence;
        }
        return(m_ulSequence);
    }

    virtual const std::string& GetWorkPath() const
    {
        return(m_strWorkPath);
    }

    virtual log4cplus::Logger GetLogger()
    {
        return(m_oLogger);
    }

    virtual const std::string& GetNodeType() const
    {
        return(m_strNodeType);
    }

    virtual const CJsonObject& GetCustomConf() const
    {
        return(m_oCustomConf);
    }

    virtual uint32 GetNodeId() const
    {
        return(m_uiNodeId);
    }

    virtual const std::string& GetHostForServer() const
    {
        return(m_strHostForServer);
    }

    virtual int GetPortForServer() const
    {
        return(m_iPortForServer);
    }

    virtual int GetWorkerIndex() const
    {
        return(m_iWorkerIndex);
    }

    virtual int GetClientBeatTime() const
    {
        return((int)m_dIoTimeout);
    }

    virtual const std::string& GetWorkerIdentify()
    {
        if (m_strWorkerIdentify.size() == 0)
        {
            char szWorkerIdentify[64] = {0};
            snprintf(szWorkerIdentify, 64, "%s:%d.%d", GetHostForServer().c_str(),
                            GetPortForServer(), GetWorkerIndex());
            m_strWorkerIdentify = szWorkerIdentify;
        }
        return(m_strWorkerIdentify);
    }

    virtual time_t GetNowTime() const;

    virtual bool Pretreat(Cmd* pCmd);
    virtual bool Pretreat(Step* pStep);
    virtual bool Pretreat(Session* pSession);
    virtual bool Register(Step* pStep, ev_tstamp dTimeout = 0.0);
    virtual bool Register(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout = 0.0);
    virtual void Remove(Step* pStep);
    virtual void Remove(uint32 uiSelfStepSeq, Step* pStep);
    //virtual bool UnRegisterCallback(Step* pStep);
    virtual bool Register(Session* pSession);
    virtual void Remove(Session* pSession);
    virtual bool Register(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep);
    virtual bool ResetTimeout(Object* pObject);
    virtual Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session");
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session");
    virtual bool Disconnect(const tagChannelContext& stCtx, bool bChannelNotice = true);
    virtual bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true);

public:     // Worker相关设置（由专用Cmd类调用这些方法完成Worker自身的初始化和更新）
    virtual bool SetProcessName(const CJsonObject& oJsonConf);
    /** @brief 加载配置，刷新Server */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel);
    virtual bool AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx);
    virtual bool AddNamedChannel(const std::string& strIdentify, Channel* pChannel);
    virtual void DelNamedChannel(const std::string& strIdentify);
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);
    virtual bool Register(const std::string& strIdentify, RedisStep* pRedisStep);
    virtual bool Register(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    /*  redis 节点管理相关操作从框架中移除，交由DataProxy的SessionRedisNode来管理，框架只做到redis的连接管理
    virtual bool RegisterCallback(const std::string& strRedisNodeType, RedisStep* pRedisStep);
    virtual void AddRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    virtual void DelRedisNodeConf(const std::string& strNodeType, const std::string strHost, int iPort);
    */
    virtual bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx);
    virtual void DelRedisContextAddr(const redisAsyncContext* ctx);

public:     // 发送数据或从Worker获取数据
    /** @brief 自动连接成功后，把待发送数据发送出去 */
    virtual bool SendTo(const tagChannelContext& stCtx);
    virtual bool SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool SendToNext(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, Object* pHttpStep = NULL);
    virtual bool SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify);
    virtual bool AutoSend(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    virtual bool AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL);
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual void SetNodeId(uint32 uiNodeId) {m_uiNodeId = uiNodeId;}
    virtual void AddInnerChannel(const tagChannelContext& stCtx);
    virtual bool SetClientData(const tagChannelContext& stCtx, const std::string& strClientData);
    virtual std::string GetClientAddr(const tagChannelContext& stCtx);
    virtual bool DiscardNamedChannel(const std::string& strIdentify);
    virtual bool SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType);
    virtual void ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,
                    int iErrno, const std::string& strErrMsg, const std::string& strErrShow);

    bool AddIoTimeout(const tagChannelContext& stCtx);

protected:
    bool Init(CJsonObject& oJsonConf);
    bool InitLogger(const CJsonObject& oJsonConf);
    bool CreateEvents();
    void PreloadCmd();
    void Destroy();
    bool AddPeriodicTaskEvent();
    bool AddIoReadEvent(int iFd);
    bool AddIoWriteEvent(int iFd);
    bool RemoveIoWriteEvent(int iFd);
    bool AddIoReadEvent(Channel* pChannel);
    bool AddIoWriteEvent(Channel* pChannel);
    bool RemoveIoWriteEvent(Channel* pChannel);
    bool AddIoTimeout(Channel* pChannel, ev_tstamp dTimeout = 1.0);
    Channel* CreateChannel(int iFd, E_CODEC_TYPE eCodecType);
    bool DiscardChannel(Channel* pChannel, bool bChannelNotice = true);
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
    bool Handle(Channel* pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    /**
     * @brief 收到完整的hhtp包后处理
     * @param stMsgShell 数据包来源消息通道
     * @param oHttpMsg 接收的HTTP包
     * @return 是否正常处理
     */
    bool Handle(Channel* pChannel, const HttpMsg& oHttpMsg);

    void LoadCmd(CJsonObject& oCmdConf);
    tagSo* LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteCmd(int iCmd);
    void LoadModule(CJsonObject& oModuleConf);
    tagModule* LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion);
    void UnloadSoAndDeleteModule(const std::string& strModulePath);

private:
    char* m_pErrBuff;
    uint32 m_ulSequence;
    log4cplus::Logger m_oLogger;
    bool m_bInitLogger;
    ev_tstamp m_dIoTimeout;             ///< IO（连接）超时配置
    ev_tstamp m_dStepTimeout;           ///< 步骤超时

    std::string m_strNodeType;          ///< 节点类型
    std::string m_strHostForServer;     ///< 对其他Server服务的IP地址（用于生成当前Worker标识）
    std::string m_strWorkerIdentify;    ///< 进程标识
    int m_iPortForServer;               ///< Server间通信监听端口（用于生成当前Worker标识）
    std::string m_strWorkPath;          ///< 工作路径
    CJsonObject m_oCustomConf;    ///< 自定义配置
    uint32 m_uiNodeId;                  ///< 节点ID
    int m_iManagerControlFd;            ///< 与Manager父进程通信fd（控制流）
    int m_iManagerDataFd;               ///< 与Manager父进程通信fd（数据流）
    int m_iWorkerIndex;
    int m_iWorkerPid;
    ev_tstamp m_dMsgStatInterval;       ///< 客户端连接发送数据包统计时间间隔
    int32  m_iMsgPermitNum;             ///< 客户端统计时间内允许发送消息数量

    int m_iRecvNum;                     ///< 接收数据包（head+body）数量
    int m_iRecvByte;                    ///< 接收字节数（已到达应用层缓冲区）
    int m_iSendNum;                     ///< 发送数据包（head+body）数量（只到达应用层缓冲区，不一定都已发送出去）
    int m_iSendByte;                    ///< 发送字节数（已到达系统发送缓冲区，可认为已发送出去）

    struct ev_loop* m_loop;
    Cmd* m_pCmdConnect;
    std::map<int, Channel*> m_mapChannel;            ///< 通信通道
    std::map<int, uint32> m_mapInnerFd;              ///< 服务端之间连接的文件描述符（用于区分连接是服务内部还是外部客户端接入）
    std::map<uint32, int> m_mapSeq2WorkerIndex;      ///< 序列号对应的Worker进程编号（用于connect成功后，向对端Manager发送希望连接的Worker进程编号）

    std::map<int32, Cmd*> m_mapPreloadCmd;                  ///< 预加载逻辑处理命令（一般为系统级命令）
    std::map<int, tagSo*> m_mapCmd;                   ///< 动态加载业务逻辑处理命令
    std::map<std::string, tagModule*> m_mapModule;   ///< 动态加载的http逻辑处理模块

    std::map<uint32, Step*> m_mapCallbackStep;
    std::map<redisAsyncContext*, tagRedisAttr*> m_mapRedisAttr;     ///< Redis连接属性
    std::map<std::string, std::map<std::string, Session*> > m_mapCallbackSession;

    /* 节点连接相关信息 */
    std::map<std::string, std::list<Channel*> > m_mapNamedChannel;      // key为Identify
    std::map<std::string, std::string> m_mapIdentifyNodeType;    // key为Identify，value为node_type
    T_MAP_NODE_TYPE_IDENTIFY m_mapNodeIdentify;

    /* redis节点信息 */
    std::map<std::string, const redisAsyncContext*> m_mapRedisContext;       ///< redis连接，key为identify(192.168.16.22:9988形式的IP+端口)
    std::map<const redisAsyncContext*, std::string> m_mapContextIdentify;    ///< redis标识，与m_mapRedisContext的key和value刚好对调
};

} /* namespace neb */

#endif /* SRC_LABOR_CHIEF_WORKER_HPP_ */
