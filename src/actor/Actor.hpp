/*******************************************************************************
 * Project:  Nebula
 * @file     Actor.hpp
 * @brief    Nebula构成实体
 * @author   Bwar
 * @date:    2016年8月9日
 * @note     Nebula构成实体
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_ACTOR_HPP_
#define SRC_ACTOR_ACTOR_HPP_

#include <memory>
#include <string>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include "ev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "Error.hpp"
#include "Definition.hpp"
#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "util/json/CJsonObject.hpp"
#include "channel/Channel.hpp"
#include "labor/Labor.hpp"
#include "ActorBuilder.hpp"

namespace neb
{

class Labor;
class Dispatcher;
class ActorBuilder;
class ActorSys;

class SocketChannel;
class RedisChannel;
class Actor;
class Cmd;
class Module;
class Session;
class Timer;
class Context;
class Step;
class Model;
class Chain;

class Actor: public std::enable_shared_from_this<Actor>
{
public:
    enum ACTOR_TYPE
    {
        ACT_UNDEFINE            = 0,        ///< 未定义
        ACT_CMD                 = 1,        ///< Cmd对象，处理带命令字的pb请求
        ACT_MODULE              = 2,        ///< Module对象，处理带url path的http请求
        ACT_SESSION             = 3,        ///< Session会话对象
        ACT_TIMER               = 4,        ///< 定时器对象
        ACT_CONTEXT             = 5,        ///< 会话上下文对象
        ACT_PB_STEP             = 6,        ///< Step步骤对象，处理pb请求或响应
        ACT_HTTP_STEP           = 7,        ///< Step步骤对象，处理http请求或响应
        ACT_REDIS_STEP          = 8,        ///< Step步骤对象，处理redis请求或响应
        ACT_MODEL              = 9,        ///< Model模型对象，Model（IO无关）与Step（异步IO相关）共同构成功能链
        ACT_CHAIN               = 10,       ///< Chain链对象，用于将Model和Step组合成功能链
    };

public:
    Actor(ACTOR_TYPE eActorType = ACT_UNDEFINE, ev_tstamp dTimeout = 0.0);
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    virtual ~Actor();

    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Context> MakeSharedContext(const std::string& strContextName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Chain> MakeSharedChain(const std::string& strChainName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Actor> MakeSharedActor(const std::string& strActorName, Targs&&... args);

    ACTOR_TYPE GetActorType() const
    {
        return(m_eActorType);
    }

    const std::string& GetActorName() const
    {
        return(m_strActorName);
    }

    const std::string& GetTraceId() const
    {
        return(m_strTraceId);
    }

    uint32 GetSequence();

protected:
    uint32 GetNodeId() const;
    uint32 GetWorkerIndex() const;
    const std::string& GetNodeType() const;
    const std::string& GetWorkPath() const;
    const std::string& GetNodeIdentify() const;
    time_t GetNowTime() const;
    ev_tstamp GetDataReportInterval() const;

    /**
     * @brief 获取Server自定义配置
     * @return Server自定义配置
     */
    const CJsonObject& GetCustomConf() const;

    std::shared_ptr<Session> GetSession(uint32 uiSessionId);
    std::shared_ptr<Session> GetSession(const std::string& strSessionId);
    bool ExecStep(uint32 uiStepSeq, int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);
    std::shared_ptr<Model> GetModel(const std::string& strModelName);
    std::shared_ptr<Context> GetContext();
    void SetContext(std::shared_ptr<Context> pContext);
    void AddAssemblyLine(std::shared_ptr<Session> pSession);

protected:
    /**
     * @brief 连接成功后发送
     * @note 当前Server往另一个Server发送数据而两Server之间没有可用连接时，框架层向对端发起连接（发起连接
     * 的过程是异步非阻塞的，connect()函数返回的时候并不知道连接是否成功），并将待发送数据存放于应用层待发
     * 送缓冲区。当连接成功时将待发送数据从应用层待发送缓冲区拷贝到应用层发送缓冲区并发送。此函数由框架层自
     * 动调用，业务逻辑层无须关注。
     * @param stCtx 消息通道上下文
     * @return 是否发送成功
     */
    bool SendTo(std::shared_ptr<SocketChannel> pChannel);

    /**
     * @brief 发送数据
     * @note 使用指定连接将数据发送。如果能直接得知stCtx（比如刚从该连接接收到数据，欲回确认包），就
     * 应调用此函数发送。此函数是SendTo()函数中最高效的一个。
     * @param stCtx 消息通道上下文
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送数据
     * @param stCtx 消息通道上下文
     * @param oHttpMsg http消息
     * @return 是否发送成功
     */
    bool SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);

    /**
     * @brief 发送数据
     * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的Channel，如果找到就调用
     * SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
     * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
     * uint32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)连接后再发送。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送数据
     * @param strHost IP地址
     * @param iPort 端口
     * @param strUrlPath 路径
     * @param oHttpMsg http消息
     * @return 是否发送成功
     */
    bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg);

    /**
     * @brief 发送到redis channel
     * @note 只有RedisStep及其派生类才能调用此方法，调用此方法时将调用者自身(RedisStep)添加到
     *       RedisChannel的pipeline中。
     * @param pChannel RedisChannel指针
     */
    bool SendTo(std::shared_ptr<RedisChannel> pChannel);

    /**
     * @brief 发送到redis channel
     * @note 只有RedisStep及其派生类才能调用此方法，调用此方法时将调用者自身(RedisStep)添加到
     *       RedisChannel的pipeline中。
     * @param strIdentify RedisChannel通道标识
     */
    bool SendTo(const std::string& strIdentify);

    /**
     * @brief 发送到redis channel
     * @note 只有RedisStep及其派生类才能调用此方法，调用此方法时将调用者自身(RedisStep)添加到
     *       RedisChannel的pipeline中。
     * @param strHost RedisChannel地址
     * @param iPort RedisChannel端口
     */
    bool SendTo(const std::string& strHost, int iPort);

    /**
     * @brief 从worker发送到loader或从loader发送到worker
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送到下一个同一类型的节点
     * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
     * @param strNodeType 节点类型
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 以取模方式选择发送到同一类型节点
     * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
     * @param strNodeType 节点类型
     * @param uiFactor 定向发送因子
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    bool SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    int32 GetStepNum() const;

protected:
    virtual void SetActiveTime(ev_tstamp dActiveTime)
    {
        m_dActiveTime = dActiveTime;
    }

    ev_tstamp GetActiveTime() const
    {
        return(m_dActiveTime);
    }

    ev_tstamp GetTimeout() const
    {
        return(m_dTimeout);
    }

private:
    void SetLabor(Labor* pLabor);
    ev_timer* MutableTimerWatcher();
    void SetActorName(const std::string& strActorName);
    void SetTraceId(const std::string& strTraceId);

private:
    ACTOR_TYPE m_eActorType;
    uint32 m_uiSequence;
    ev_tstamp m_dActiveTime;
    ev_tstamp m_dTimeout;
    Labor* m_pLabor;
    ev_timer* m_pTimerWatcher;
    std::string m_strActorName;
    std::string m_strTraceId;       // for log trace
    std::shared_ptr<Context> m_pContext;

    friend class Dispatcher;
    friend class ActorBuilder;
    friend class ActorSys;
    friend class Chain;
};

template <typename ...Targs>
void Actor::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLabor->GetActorBuilder()->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Actor::MakeSharedStep(const std::string& strStepName, Targs&&... args)
{
    return(m_pLabor->GetActorBuilder()->MakeSharedStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Actor::MakeSharedSession(const std::string& strSessionName, Targs&&... args)
{
    return(m_pLabor->GetActorBuilder()->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Context> Actor::MakeSharedContext(const std::string& strContextName, Targs&&... args)
{
    return(m_pLabor->GetActorBuilder()->MakeSharedContext(this, strContextName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Actor> Actor::MakeSharedActor(const std::string& strActorName, Targs&&... args)
{
    return(m_pLabor->GetActorBuilder()->MakeSharedActor(this, strActorName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Chain> Actor::MakeSharedChain(const std::string& strChainName, Targs&&... args)
{
    return(m_pLabor->GetActorBuilder()->MakeSharedChain(this, strChainName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_ACTOR_HPP_ */
