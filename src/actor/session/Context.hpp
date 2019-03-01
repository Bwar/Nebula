/*******************************************************************************
 * Project:  Nebula
 * @file     Context.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_SESSION_CONTEXT_HPP_
#define SRC_ACTOR_SESSION_CONTEXT_HPP_

#include "labor/Worker.hpp"
#include "actor/DynamicCreator.hpp"
#include "Session.hpp"

namespace neb
{

/**
 * @brief Actor上下文信息
 * @note Actor上下文信息用于保存Actor（主要是Step，也可用于Session）运行
 * 过程中的上下信息存储和数据共享。
 *     比如Cmd收到一个消息，后续需要若干个Step才完成全部操作，在最后一个
 * Step中给请求方发送响应，那么最后一个Step需要获取Cmd当时收到的请求上下
 * 文，而最后一个Step可能不是Cmd所创建的，一种实现方式是将上下文信息顺着
 * Step链条赋值传递（因为前面的Step执行完毕可能会被销毁，不能用引用传递）
 * 下去，这样会导致多次内存拷贝。Context的存在是为了减少这种不必要的复制，
 * Context由框架管理，Step只需传递Context的引用。
 *     从Context基类派生出业务自己的Context类可以用于Step间自定义数据的共
 * 享（当然，Session类也可以达到同样目的，区别在于GetSession()需要传入
 * SessionId才能获取到Session，GetContext()则不需要传参即可获取）。什么时
 * 候用Context？建议是同一个StepChain（共同完成一个操作的Step执行链）的上
 * 下文信息和数据共享用Context，其他情况用Session。
 */
class Context: public Session
{
public:
    Context(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Context(const std::string& strSessionId,
            std::shared_ptr<SocketChannel> pChannel,
            int32 iCmd, uint32 uiSeq,
            ev_tstamp dSessionTimeout = 60.0);
    Context(const std::string& strSessionId,
            std::shared_ptr<SocketChannel> pChannel,
            int32 iCmd, uint32 uiSeq,
            const MsgBody& oReqMsgBody,
            ev_tstamp dSessionTimeout = 60.0);
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    virtual ~Context();

    /**
     * @brief 会话超时回调
     * @note 上下文由Worker进行管理，超时回调时一般默认返回CMD_STATUS_COMPLETED，
     * 让框架执行回收操作即可（实际上因为上下文以shared_ptr存储，并未在框架执行
     * 回收操作时立即销毁，而是在所有引用该Context的Actor实例均已销毁时才会销毁）。
     */
    virtual E_CMD_STATUS Timeout()
    {
        return(CMD_STATUS_COMPLETED);
    }

public:
    /**
     * @brief 给请求方发响应
     * @param iErrno 错误码，如果正确则应传入ERR_OK
     * @param strData 响应结果（错误码为ERR_OK时），或错误信息
     */
    bool Response(int iErrno, const std::string& strData);

    std::shared_ptr<SocketChannel> GetChannel()
    {
        return(m_pChannel);
    }

    int32 GetCmd() const
    {
        return(m_iReqCmd);
    }

    uint32 GetSeq() const
    {
        return(m_uiReqSeq);
    }

    const MsgBody& GetMsgBody() const
    {
        return(m_oReqMsgBody);
    }

private:
    std::shared_ptr<SocketChannel> m_pChannel;
    int32 m_iReqCmd;
    uint32 m_uiReqSeq;
    MsgBody m_oReqMsgBody;
    
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_CONTEXT_HPP_ */
