/*******************************************************************************
 * Project:  Nebula
 * @file     Object.hpp
 * @brief    Nebula构成实体
 * @author   Bwar
 * @date:    2016年8月9日
 * @note     Nebula构成实体
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_OBJECT_HPP_
#define SRC_OBJECT_OBJECT_HPP_

#include <string>

#include "log4cplus/logger.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/loggingmacros.h"
#include "Error.hpp"
#include "Definition.hpp"
#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "labor/Labor.hpp"
#include "channel/Channel.hpp"

namespace neb
{

enum OBJECT_TYPE
{
    OBJ_UNDEFINE            = 0,        ///< 未定义
    OBJ_CMD                 = 1,        ///< Cmd对象，处理带命令字的pb请求
    OBJ_MODULE              = 2,        ///< Module对象，处理带url path的http请求
    OBJ_SESSION             = 3,        ///< Session会话对象
    OBJ_TIMER               = 4,        ///< 定时器对象
    OBJ_PB_STEP             = 5,        ///< Step步骤对象，处理pb请求或响应
    OBJ_HTTP_STEP           = 6,        ///< Step步骤对象，处理http请求或响应
    OBJ_REDIS_STEP          = 7,        ///< Step步骤对象，处理redis请求或响应
};

class Worker;
class CmdUpdateNodeId;
class StepIoTimeout;

class Object
{
public:
    Object(OBJECT_TYPE eObjectType);
    virtual ~Object();

    virtual std::string ObjectName() const = 0;

protected:
    // template <typename T> T* NewObject(T);

    /**
     * @brief 预处理
     * @note 预处理用于将等待预处理对象与框架建立弱联系，使被处理的对象可以获得框架一些工具，如写日志指针
     * @param pObject 等待预处理的对象
     * @return 预处理结果
     */
    bool Pretreat(Object* pObject);

    /**
     * @brief 登记对象
     * @note  若Register()登记失败（失败可能性微乎其微）则应将被登记对象销毁,重新new 对象并登记，
     * 若多次（可自定义）登记失败则应放弃登记，并将对象销毁；若Register()登记成功则一定不可以
     * 销毁（销毁操作会自动在回调之后执行）
     * @param pObject 对象
     * @param dTimeout 超时时间
     * @return 是否登记成功
     */
    bool Register(Object* pObject, ev_tstamp dTimeout = 0.0);

    /**
     * @brief 注销对象
     * @note 注销对象会同时销毁该对象
     * @param pObject 对象
     */
    void Remove(Object* pObject);

protected:
    /**
     * @brief 获取当前Step的序列号
     * @note 获取当前Step的序列号，对于同一个Step实例，每次调用GetSequence()获取的序列号是相同的。
     * @return 序列号
     */
    uint32 GetSequence();
    uint32 GetTraceId() const
    {
        return(m_ulTraceId);
    }
    uint32 GetNodeId() const;
    uint32 GetWorkerIndex() const;
    const std::string& GetNodeType() const;
    const std::string& GetWorkPath() const;

    /**
     * @brief 获取当前Worker进程标识符
     * @note 当前Worker进程标识符由 IP:port:worker_index组成，例如： 192.168.18.22:30001.2
     * @return 当前Worker进程标识符
     */
    const std::string& GetWorkerIdentify();

    /**
     * @brief 获取Server自定义配置
     * @return Server自定义配置
     */
    const CJsonObject& GetCustomConf() const;

    /**
     * @brief 获取当前时间
     * @note 获取当前时间，比time(NULL)速度快消耗小，不过没有time(NULL)精准，如果对时间精度
     * 要求不是特别高，建议调用GetNowTime()替代time(NULL)
     * @return 当前时间
     */
    time_t GetNowTime() const;

    /**
     * @brief 获取会话实例
     * @param uiSessionId 会话ID
     * @return 会话实例（返回NULL表示不存在uiSessionId对应的会话实例）
     */
    Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "oss::Session");
    Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "oss::Session");

    /**
     * @brief 添加指定标识的消息通道上下文
     * @note 添加指定标识的消息外壳由Cmd类实例和Step类实例调用，该调用会在Step类中添加一个标识
     * 和消息外壳的对应关系，同时Worker中的连接属性也会添加一个标识。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param stCtx  消息通道上下文
     * @return 是否添加成功
     */
    bool AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx);

    /**
     * @brief 删除指定标识的消息通道上下文
     * @note 删除指定标识的消息通道上下文由Worker类实例调用，在IoError或IoTimeout时调
     * 用。
     */
    void DelNamedChannel(const std::string& strIdentify);

    /**
     * @brief 添加内部服务器通信通道
     * @param stCtx 通信通道上下文
     */
    void AddInnerChannel(const tagChannelContext& stCtx);

    /**
     * @brief 添加标识的节点类型属性
     * @note 添加标识的节点类型属性，用于以轮询方式向同一节点类型的节点发送数据，以
     * 实现简单的负载均衡。只有Server间的各连接才具有NodeType属性，客户端到Access节
     * 点的连接不具有NodeType属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    void AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);

    /**
     * @brief 删除标识的节点类型属性
     * @note 删除标识的节点类型属性，当一个节点下线，框架层会自动调用此函数删除标识
     * 的节点类型属性。
     * @param strNodeType 节点类型
     * @param strIdentify 连接标识符
     */
    void DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify);

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
    bool SendTo(const tagChannelContext& stCtx);

    /**
     * @brief 发送数据
     * @note 使用指定连接将数据发送。如果能直接得知stCtx（比如刚从该连接接收到数据，欲回确认包），就
     * 应调用此函数发送。此函数是SendTo()函数中最高效的一个。
     * @param stCtx 消息通道上下文
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    /**
     * @brief 发送数据
     * @param stCtx 消息通道上下文
     * @param oHttpMsg http消息
     * @return 是否发送成功
     */
    bool SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg);

    /**
     * @brief 发送数据
     * @note 指定连接标识符将数据发送。此函数先查找与strIdentify匹配的stMsgShell，如果找到就调用
     * SendTo(const tagMsgShell& stMsgShell, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
     * 发送，如果未找到则调用SendToWithAutoConnect(const std::string& strIdentify,
     * uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)连接后再发送。
     * @param strIdentify 连接标识符(IP:port.worker_index, e.g 192.168.11.12:3001.1)
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);

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
     * @brief 发送到下一个同一类型的节点
     * @note 发送到下一个同一类型的节点，适用于对同一类型节点做轮询方式发送以达到简单的负载均衡。
     * @param strNodeType 节点类型
     * @param uiCmd 发送的命令字
     * @param uiSeq 发送的数据包seq
     * @param oMsgBody 数据包体
     * @return 是否发送成功
     */
    bool SendToNext(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);

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
    bool SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody);

protected:
    virtual void SetActiveTime(ev_tstamp dActiveTime)
    {
        m_dActiveTime = dActiveTime;
    }

    ev_tstamp GetActiveTime() const
    {
        return(m_dActiveTime);
    }

    void SetTimeout(ev_tstamp dTimeout)
    {
        m_dTimeout = dTimeout;
    }

    ev_tstamp GetTimeout() const
    {
        return(m_dTimeout);
    }

    bool IsRegistered() const
    {
        return(m_bRegistered);
    }

    OBJECT_TYPE GetObjectType() const
    {
        return(m_eObjectType);
    }

    /**
     * @brief 获取日志类实例
     * @note 派生类写日志时调用
     * @return 日志类实例
     */
    log4cplus::Logger GetLogger()
    {
        return (*m_pLogger);
    }

    log4cplus::Logger* GetLoggerPtr()
    {
        return (m_pLogger);
    }

    bool SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType);

private:
    /**
     * @brief 设置框架层操作者
     * @note 设置框架层操作者，由框架层调用，业务层派生类可直接忽略此函数
     * @param pLabor 框架层操作者，一般为OssWorker的实例
     */
    void SetLabor(Labor* pLabor)
    {
        m_pLabor = pLabor;
    }

    /**
     * @brief 设置日志类实例
     * @note 设置框架层操作者，由框架层调用，业务层派生类可直接忽略此函数
     * @param logger 日志类实例
     */
    void SetLogger(log4cplus::Logger* pLogger)
    {
        m_pLogger = pLogger;
    }

    void SetRegistered()
    {
        m_bRegistered = true;
    }

    void SetNodeId(uint32 uiNodeId);

    void DelayTimeout();

    void SetTraceId(uint32 ulTraceId)
    {
        m_ulTraceId = ulTraceId;
    }

    ev_timer* AddTimerWatcher();
    ev_timer* MutableTimerWatcher()
    {
        return(m_pTimerWatcher);
    }

private:
    OBJECT_TYPE m_eObjectType;
    bool m_bRegistered;
    uint32 m_ulSequence;
    uint32 m_ulTraceId;
    ev_tstamp m_dActiveTime;
    ev_tstamp m_dTimeout;
    std::string m_strWorkerIdentify;
    Labor* m_pLabor;
    log4cplus::Logger* m_pLogger;
    ev_timer* m_pTimerWatcher;

    friend class Worker;
    friend class CmdUpdateNodeId;
    friend class StepIoTimeout;
};

} /* namespace neb */

#endif /* SRC_OBJECT_OBJECT_HPP_ */
