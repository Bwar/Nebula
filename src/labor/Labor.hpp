/*******************************************************************************
 * Project:  Nebula
 * @file     OssLabor.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月8日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LABOR_LABOR_HPP_
#define LABOR_LABOR_HPP_

#include "util/json/CJsonObject.hpp"
#include "pb/msg.pb.h"
#include "Definition.hpp"

namespace neb
{

class Labor
{
public:
    Labor(){};
    Labor(const Labor&) = delete;
    Labor& operator=(const Labor&) = delete;
    virtual ~Labor(){};

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
};

} /* namespace neb */

#endif /* LABOR_LABOR_HPP_ */
