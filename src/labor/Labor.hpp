/*******************************************************************************
 * Project:  Nebula
 * @file     OssLabor.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月8日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_LABOR_HPP_
#define SRC_LABOR_LABOR_HPP_

#include "ev.h"
#include "hiredis/async.h"
#include "util/json/CJsonObject.hpp"
#include "util/CBuffer.hpp"
#include "Error.hpp"
#include "Definition.hpp"
#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "codec/Codec.hpp"

namespace neb
{

class Object;
class Cmd;
class Module;
class Session;
class Timer;
class Step;
class RedisStep;
class HttpStep;

class Labor
{
public:
    Labor();
    virtual ~Labor();

public:     // Labor相关设置（由Cmd类或Step类调用这些方法完成Labor自身的初始化和更新，Manager和Worker类都必须实现这些方法）
    /**
     * @brief 设置进程名
     * @param oJsonConf 配置信息
     * @return 是否设置成功
     */
    virtual bool SetProcessName(const CJsonObject& oJsonConf) = 0;

    /** @brief 设置日志级别 */
    virtual void ResetLogLevel(log4cplus::LogLevel iLogLevel) = 0;

    /**
     * @brief 连接成功后发送
     * @note 当前Server往另一个Server发送数据而两Server之间没有可用连接时，框架层向对端发起连接（发起连接
     * 的过程是异步非阻塞的，connect()函数返回的时候并不知道连接是否成功），并将待发送数据存放于应用层待发
     * 送缓冲区。当连接成功时将待发送数据从应用层待发送缓冲区拷贝到应用层发送缓冲区并发送。此函数由框架层自
     * 动调用，业务逻辑层无须关注。
     * @param stCtx 消息通道上下文
     * @return 是否发送成功
     */
    virtual bool SendTo(const tagChannelContext& stCtx) = 0;

    /**
     * @brief 发送数据
     * @note 使用指定连接将数据发送。如果能直接得知stMsgShell（比如刚从该连接接收到数据，欲回确认包），就
     * 应调用此函数发送。此函数是SendTo()函数中最高效的一个。
     * @param stCtx 消息通道上下文
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody) = 0;

    /**
     * @brief 设置连接的标识符信息
     * @note 当连接断开时，框架层工作者可以通过连接属性里的strIdentify调用Step::DelChannelContext(
     * const std::string& strIdentify)删除连接标识。
     * @param stCtx 消息通道上下文
     * @param strIdentify 连接标识符
     * @return 是否设置成功
     */
    virtual bool SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify) = 0;

    /**
     * @brief 自动连接并发送
     * @note 当strIdentify对应的连接不存在时，分解strIdentify得到host、port等信息建立连接，连接成功后发
     * 送数据。仅适用于strIdentify是合法的Server间通信标识符（IP:port:worker_index组成）。返回ture只标
     * 识连接这个动作发起成功，不代表数据已发送成功。
     * @param strIdentify 连接标识符
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 是否可以自动发送
     */
    virtual bool AutoSend(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody) = 0;

    // TODO virtual bool AutoHttp(const std::string& strHost, int iPort, const HttpMsg& oHttpMsg);

    /**
     * @brief 自动连接并执行redis命令
     * @param strHost redis服务所在IP
     * @param iPort redis端口
     * @param pRedisStep 执行redis命令的redis步骤实例
     * @return 是否可以执行
     */
    virtual bool AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep) = 0;

    /**
     * @brief 设置节点ID
     * @param iNodeId 节点ID
     */
    virtual void SetNodeId(uint32 uiNodeId) = 0;

    /**
     * @brief 添加内部通信连接信息
     * @param stCtx 消息通道上下文
     */
    virtual void AddInnerChannel(const tagChannelContext& stCtx) = 0;

    virtual uint32 GetNodeId() const = 0;

public:     // Worker相关设置（由Cmd类或Step类调用这些方法完成数据交互，Worker类必须重新实现这些方法，Manager类可不实现）
    /**
     * @brief 获取序列号
     * @return 序列号
     */
    virtual uint32 GetSequence()
    {
        return(0);
    }

    /**
     * @brief 获取工作目录
     * @return 工作目录
     */
    virtual const std::string& GetWorkPath() const
    {
        return(m_strNodeTypeTmp);
    }

    /**
     * @brief 获取日志实例
     * @return 日志实例
     */
    virtual log4cplus::Logger GetLogger()
    {
        log4cplus::Logger oLogger;
        return (oLogger);
    }

    /**
     * @brief 获取节点类型
     * @return 节点类型
     */
    virtual const std::string& GetNodeType() const
    {
        return(m_strNodeTypeTmp);
    }

    /**
     * @brief 获取自定义配置
     * @return 自定义配置
     */
    virtual const CJsonObject& GetCustomConf() const
    {
        return(m_oCustomConfTmp);
    }

    /**
     * @brief 获取Server间的连接IP
     * @return Server间的连接IP
     */
    virtual const std::string& GetHostForServer() const
    {
        return(m_strHostForServerTmp);
    }

    /**
     * @brief 获取Server间的连接端口
     * @return Server间的连接端口
     */
    virtual int GetPortForServer() const
    {
        return(0);
    }

    /**
     * @brief 获取Worker进程编号
     * @return Worker进程编号
     */
    virtual int GetWorkerIndex() const
    {
        return(0);
    }

    /**
     * @brief 获取客户端连接心跳时间
     * @return 客户端连接心跳时间
     */
    virtual int GetClientBeatTime() const
    {
        return(300);
    }

    virtual bool IoTimeout(ev_timer* watcher, bool bCheckBeat = true)
    {
        return(false);
    }

    /**
     * @brief 获取当前时间
     * @note 获取当前时间，比time(NULL)速度快消耗小，不过没有time(NULL)精准，如果对时间精度
     * 要求不是特别高，建议调用GetNowTime()替代time(NULL)
     * @return 当前时间
     */
    virtual time_t GetNowTime() const
    {
        return(time(NULL));
    }

    /**
     * @brief 延迟Object超时时间（重新设置超时时间）
     * @param pObject 被延迟的Object
     * @return 是否设置成功
     */
    virtual bool ResetTimeout(Object* pObject)
    {
        return(false);
    }

    /**
     * @brief 预处理
     * @note 预处理用于将等待预处理对象与框架建立弱联系，使被处理的对象可以获得框架一些工具，如写日志指针
     * @param pObject 等待预处理的对象
     * @return 预处理结果
     */
    virtual bool Pretreat(Object* pObject)
    {
        return(false);
    }

    /**
     * @brief 注册步骤回调
     * @param pStep 步骤回调
     * @return 是否注册成功
     */
    virtual bool Register(Step* pStep, double dTimeout = 0.0)
    {
        return(false);
    }

    /**
     * @brief 注册步骤回调
     * @param uiSelfStepSeq 调用注册step的当前step seq
     * @param pStep 步骤回调
     * @param dTimeout 超时时间
     * @return 是否注册成功
     */
    virtual bool Register(uint32 uiSelfStepSeq, Step* pStep, double dTimeout = 0.0)
    {
        return(false);
    }

    /**
     * @brief 删除步骤回调
     * @param pStep 步骤回调
     */
    virtual void Remove(Step* pStep)
    {
    }

    /**
     * @brief 删除步骤回调
     * @param 执行删除操作的当前步骤seq
     * @param pStep 步骤回调
     */
    virtual void Remove(uint32 uiSelfStepSeq, Step* pStep)
    {
    }

    /**
     * @brief 注册会话
     * @param pSession 会话
     * @return 是否注册成功
     */
    virtual bool Register(Session* pSession)
    {
        return(false);
    }

    /**
     * @brief 删除会话
     * @param pSession 会话
     */
    virtual void Remove(Session* pSession)
    {
    }

    /**
     * @brief 注册redis回调
     * @param pRedisContext redis连接上下文
     * @param pRedisStep redis步骤
     * @return 是否注册成功
     */
    virtual bool Register(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep)
    {
        return(false);
    }

    /**
     * @brief 获取Session实例
     * @param uiSessionId 会话ID
     * @return 会话实例（返回NULL表示不存在uiSessionId对应的会话实例）
     */
    virtual Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session")
    {
        return(NULL);
    }
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session")
    {
        return(NULL);
    }

    /**
     * @brief 添加指定标识的消息通道上下文
     * @note 添加指定标识的消息通道上下文由Cmd类实例和Step类实例调用，该调用会在Step类中添加一个标识
     * 和消息通道上下文的对应关系，同时Worker中的连接属性也会添加一个标识。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param stCtx  消息通道上下文
     * @return 是否添加成功
     */
    virtual bool AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx)
    {
        return(false);
    }

    /**
     * @brief 删除指定标识的消息通道上下文
     * @note 删除指定标识的消息通道上下文由Worker类实例调用，在IoError或IoTimeout时调
     * 用。
     */
    virtual void DelNamedChannel(const std::string& strIdentify)
    {
    }

    /**
     * @brief 添加标识的节点类型属性
     * @note 添加标识的节点类型属性，用于以轮询方式向同一节点类型的节点发送数据，以
     * 实现简单的负载均衡。只有Server间的各连接才具有NodeType属性，客户端到Access节
     * 点的连接不具有NodeType属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    virtual void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
    {
    }

    /**
     * @brief 删除标识的节点类型属性
     * @note 删除标识的节点类型属性，当一个节点下线，框架层会自动调用此函数删除标识
     * 的节点类型属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    virtual void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
    {
    }

    /**
     * @brief 注册redis回调
     * @param strIdentify redis节点标识(192.168.16.22:9988形式的IP+端口)
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    virtual bool Register(const std::string& strIdentify, RedisStep* pRedisStep)
    {
        return(false);
    }

    /**
     * @brief 注册redis回调
     * @param strHost redis节点IP
     * @param iPort redis端口
     * @param pRedisStep redis步骤实例
     * @return 是否注册成功
     */
    virtual bool Register(const std::string& strHost, int iPort, RedisStep* pRedisStep)
    {
        return(false);
    }

    /**
     * @brief 添加指定标识的redis context地址
     * @note 添加指定标识的redis context由Worker调用，该调用会在Step类中添加一个标识
     * 和redis context的对应关系。
     */
    virtual bool AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
    {
        return(false);
    }

    /**
     * @brief 删除指定标识的redis context地址
     * @note 删除指定标识的到redis地址的对应关系（此函数被调用时，redis context的资源已被释放或将被释放）
     * 用。
     */
    virtual void DelRedisContextAddr(const redisAsyncContext* ctx)
    {
    }

    /**
     * @brief 获取连接
     * @param strIdentify 连接标识符
     * @param stCtx 连接通道
     * @return 连接通道
     */
    virtual bool GetChannelContext(const std::string& strIdentify, tagChannelContext& stCtx)
    {
        return(false);
    }

    /**
     * @breif 设置客户端连接相关数据
     * @param oBuff 客户端连接相关数据
     * @return 是否设置成功
     */
    virtual bool SetClientData(const tagChannelContext& stCtx, CBuffer* pBuff)
    {
        return(false);
    }

    /**
     * @brief 判断是否存在客户端数据
     * @param stCtx 客户端连接通道
     * @return 是否存在客户端数据
     */
    virtual bool HadClientData(const tagChannelContext& stCtx)
    {
        return(false);
    }

    virtual std::string GetClientAddr(const tagChannelContext& stCtx)
    {
        return("");
    }

    /**
     * @brief 发送数据
     * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的stMsgShell，如果找到就调用
     * SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
     * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
     * uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)连接后再发送。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(false);
    }

    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, Object* pHttpStep = NULL)
    {
        return(false);
    }

    /**
     * @brief 发送数据
     * @param stCtx 消息通道上下文
     * @param oHttpMsg Http数据包
     * @return 是否发送成功
     */
    virtual bool SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, HttpStep* pHttpStep = NULL)
    {
        return(false);
    }

    virtual bool AutoConnect(const std::string& strIdentify)
    {
        return(false);
    }

    /**
     * @brief 发送到下一个同一类型的节点
     * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
     * @param strNodeType 节点类型
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendToNext(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(false);
    }

    /**
     * @brief 以取模方式选择发送到同一类型节点
     * @note 以取模方式选择发送到同一类型节点，实现简单有要求的负载均衡。
     * @param strNodeType 节点类型
     * @param uiModFactor 取模因子
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(false);
    }

    /**
     * @brief 发送到一种类型的节点
     * @note 发送到同一种类型除当前节点之外的所有节点。
     * @param strNodeType 节点类型
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    virtual bool Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(false);
    }

    /**
     * @brief 断开连接
     * @note 当业务层发现连接非法（如客户端登录时无法通过验证），可调用此方法断开连接
     * @param stCtx 消息通道上下文
     * @return 断开连接结果
     */
    virtual bool Disconnect(const tagChannelContext& stCtx, bool bChannelNotice = true)
    {
        return(false);
    }

    /**
     * @brief 断开连接
     * @note 当业务层发现连接非法（如客户端登录时无法通过验证），可调用此方法断开连接
     * @param strIdentify 连接标识符
     * @return 断开连接结果
     */
    virtual bool Disconnect(const std::string& strIdentify, bool bChannelNotice = true)
    {
        return(false);
    }

    /**
     * @brief 放弃已命名的连接
     * @note 放弃已存在连接是告知框架将连接标识符与连接的MsgShell的关系解除掉，并且将连接
     * 置为未验证状态，但并不实际断开连接。断开连接由后续步骤完成，或者等待超时断开。使用场
     * 景之一：已登录的用户在另一个终端重复登录，将之前的终端踢下线，踢下线的过程是先放弃连
     * 接，等客户端对踢下线消息响应或超时再实际把连接断开。
     * @param strIdentify 已存在连接的连接标识符
     * @return 放弃结果
     */
    virtual bool DiscardNamedChannel(const std::string& strIdentify)
    {
        return(false);
    }

    /**
     * @brief 转换编解码器
     * @param stCtx  消息通道上下文
     * @param eCodecType 目标编解码器
     * @return 是否成功转换
     */
    virtual bool SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType)
    {
        return(false);
    }

    /**
     * @brief 执行下一步
     * @param uiCallerStepSeq  调用者step seq
     * @param uiCalledStepSeq  下一步对应（被调用）的seq
     * @param iErrno     错误码
     * @param strErrMsg  错误信息
     * @param strErrShow 展示给用户的错误信息
     */
    virtual void ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,
                    int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
    {
        ;
    }

private:
    std::string m_strNodeTypeTmp;
    std::string m_strHostForServerTmp;
    CJsonObject m_oCustomConfTmp;
};

} /* namespace neb */

#endif /* SRC_LABOR_LABOR_HPP_ */
