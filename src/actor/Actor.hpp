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
#include "pb/redis.pb.h"
#include "util/json/CJsonObject.hpp"
#include "channel/Channel.hpp"
#include "labor/Labor.hpp"
#include "codec/Codec.hpp"
#include "ActorBuilder.hpp"
#include "ActorSender.hpp"

namespace neb
{

typedef RedisReply RedisMsg;

class Labor;
class Dispatcher;
class ActorBuilder;
class ActorSender;
class ActorSys;

class SocketChannel;
class Actor;
class Cmd;
class Module;
class Session;
class Timer;
class Context;
class Step;
class Operator;
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
        ACT_RAW_STEP            = 9,        ///< Step步骤对象，处理未经编解码的裸数据
        ACT_OPERATOR            = 10,       ///< Operator算子对象，Operator（IO无关）与Step（异步IO相关）共同构成功能链
        ACT_CHAIN               = 11,       ///< Chain链对象，用于将Operator和Step组合成功能链
    };

public:
    Actor(ACTOR_TYPE eActorType = ACT_UNDEFINE, ev_tstamp dTimeout = 0.0);
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    virtual ~Actor();

    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args) const;
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
    const NodeInfo& GetNodeInfo() const;
    time_t GetNowTime() const;
    long GetNowTimeMs() const;
    ev_tstamp GetDataReportInterval() const;

    /**
     * @brief 获取Server自定义配置
     * @return Server自定义配置
     */
    const CJsonObject& GetCustomConf() const;

    std::shared_ptr<Session> GetSession(uint32 uiSessionId);
    std::shared_ptr<Session> GetSession(const std::string& strSessionId);
    bool ExecStep(uint32 uiStepSeq, int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);
    std::shared_ptr<Operator> GetOperator(const std::string& strOperatorName);
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
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel);

    /**
     * @brief 发送PB响应数据
     * @note 使用指定连接将数据发送
     * @param pChannel 消息通道
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送HTTP响应
     * @param pChannel 消息通道
     * @param oHttpMsg http消息
     * @return 是否发送成功
     */
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);

    /**
     * @brief 发送redis响应
     * @param pChannel 消息通道
     * @param oRedisReply redis消息
     * @return 是否发送成功
     */
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, const RedisReply& oRedisReply);

    /**
     * @brief 发送raw响应
     * @param pChannel 消息通道
     * @param pRawData raw消息
     * @param uiRawDataSize raw消息长度
     * @return 是否发送成功
     */
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, const char* pRawData, uint32 uiRawDataSize);

    /**
     * @brief 发送请求
     * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的Channel，如果找到就调用
     * SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
     * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
     * uint32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)连接后再发送。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @param oCodecType 编解码方式
     * @return 是否发送成功
     */
    virtual bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq,
            const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    /**
     * @brief 发送http请求
     * @param strHost IP地址
     * @param iPort 端口
     * @param oHttpMsg http消息
     * @param uiStepSeq 应用层无用参数，框架层的系统Actor会用到
     * @return 是否发送成功
     */
    virtual bool SendTo(const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq = 0);

    /**
     * @brief 发送redis请求
     * @note 只有RedisStep及其派生类才能调用此方法
     * @param strIdentify RedisChannel通道标识，格式如： 192.168.125.53:6379
     * @param oRedisMsg redis消息
     * @param bWithSsl 是否需要SSL
     * @param bPipeline 是否支持pipeline
     * @param uiStepSeq 应用层无用参数，框架层的系统Actor会用到
     * @return 是否发送成功
     */
    virtual bool SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = true, uint32 uiStepSeq = 0);

    /**
     * @brief 发送redis请求
     * @note 只有RedisStep及其派生类才能调用此方法
     * @param strIdentify RedisChannel通道标识，格式如： 192.168.125.53:6379
     * @param oRedisMsg redis消息
     * @param bWithSsl 是否需要SSL
     * @param bPipeline 是否支持pipeline
     * @param bEnableReadOnly redis cluster集群从节点，官方默认设置的是不分担读请求
     * 只作备份和故障转移用，当有请求读向从节点时，会被重定向对应的主节点来处理。
     * 这个readonly告诉redis cluster从节点客户端愿意读取可能过时的数据并对写请求不感兴趣
     * @return 是否发送成功
     */
    virtual bool SendToCluster(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = true, bool bEnableReadOnly = false);
    /**
     * @brief 发送redis请求到类似于codis proxy的服务
     */
    virtual bool SendRoundRobin(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = false);

    /**
     * @brief 发送raw请求
     * @note 只有RawStep及其派生类才能调用此方法。
     * @param strIdentify Channel通道标识
     * @param pRawData裸数据
     * @param bWithSsl 是否需要SSL
     * @param bPipeline 是否支持pipeline
     * @param uiStepSeq 应用层无用参数，框架层的系统Actor会用到
     * @return 是否发送成功
     */
    virtual bool SendTo(const std::string& strIdentify, const char* pRawData, uint32 uiRawDataSize, bool bWithSsl = false, bool bPipeline = false, uint32 uiStepSeq = 0);

    /**
     * @brief 从worker发送到loader或从loader发送到worker
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendTo(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送到下一个同一类型的节点
     * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
     * @param strNodeType 节点类型
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @param oCodecType 编解码方式
     * @return 是否发送成功
     */
    virtual bool SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    /**
     * @brief 以取模方式选择发送到同一类型节点
     * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
     * @param strNodeType 节点类型
     * @param uiFactor 定向发送因子
     * @param iCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @param oCodecType 编解码方式
     * @return 是否发送成功
     */
    virtual bool SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    virtual bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    virtual bool SendDataReport(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 断路器
     * @note 连接错误框架会自动熔断，如果需要主动熔断（比如超时率太高）可主动调用此函数进行熔断。
     */
    void CircuitBreak(const std::string& strIdentify);

    /**
     * @brief 发送请求到当前worker
     */
    bool SendToSelf(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendToSelf(const HttpMsg& oHttpMsg);
    bool SendToSelf(const RedisMsg& oRedisMsg);
    bool SendToSelf(const char* pRaw, uint32 uiRawSize);

    std::shared_ptr<SocketChannel> GetLastActivityChannel();

    /**
     * @brief 关闭raw数据通道
     * @note 当且仅当raw数据传输的无编解码通道才可以由Actor的应用层关闭，对于http连接
     * 等只应由框架层管理的通道，调用此函数不能关闭连接，也不应该尝试自己去关闭。
     */
    virtual bool CloseRawChannel(std::shared_ptr<SocketChannel> pChannel);

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
    uint32 ForceNewSequence();
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
    friend class ActorSender;
    friend class Chain;
};

template <typename ...Targs>
void Actor::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args) const
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
