/*******************************************************************************
 * Project:  Nebula
 * @file     Context.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CONTEXT_HPP_
#define SRC_ACTOR_CONTEXT_HPP_

#include "actor/DynamicCreator.hpp"
#include "actor/Actor.hpp"

namespace neb
{

class ActorBuilder;

/**
 * @brief Actor上下文信息
 * @note Actor上下文信息用于保存Actor（主要是Step，也可用于Session）运行
 * 过程中的上下信息存储和数据共享。
 *     Context由Actor（可以是Cmd、Module、Step、Chain）创建，并由创建者
 * Actor和传递使用者Actor之间共享，框架不登记和持有；当没有一个Actor持有
 * Context时，std::shared_ptr<Context>引用计数为0将自动回收。
 *     比如Cmd收到一个消息，后续需要若干个Step才完成全部操作，在最后一个
 * Step中给请求方发送响应，那么最后一个Step需要获取Cmd当时收到的请求上下
 * 文，而最后一个Step可能不是Cmd所创建的，一种实现方式是将上下文信息顺着
 * Step链条赋值传递（因为前面的Step执行完毕可能会被销毁，不能用引用传递）
 * 下去，这样会导致多次内存拷贝。Context的存在是为了减少这种不必要的复制，
 * Context由框架管理，Step只需传递Context的引用。
 *     从Context基类派生出业务自己的Context类可以用于Step间自定义数据的共
 * 享（当然，Session类也可以达到同样目的，区别在于GetSession()需要传入
 * SessionId才能获取到Session，GetContext()则不需要传参即可获取）。什么时
 * 候用Context？建议是同一个Chain（共同完成一个操作的Step执行链）的上
 * 下文信息和数据共享用Context，其他情况用Session。
 */
class Context: public Actor
{
public:
    Context();
    Context(std::shared_ptr<SocketChannel> pChannel);
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    virtual ~Context();

    /**
     * @brief 动态处理链上下文操作完成
     * @note 一次客户端请求可能需要若干步骤（Step）才能完成，当处理完成时，
     * 对于预定义的静态请求处理链,通常会由最后一个步骤（Step）给客户端发响
     * 应，响应信息通常来自于各步骤存储于Context的数据。然而，对于通过配置
     * 完成的动态请求处理链，最后一个步骤通常是未知的，如果想要由最后一个步
     * 骤发送响应，那么无论处理链有多少个步骤，都需要固定最后一个步骤，这是
     * 不合理的，配置动态处理链的时候也容易出错。因此，在Context中引入一个
     * Done()方法，把给客户端的响应放在Done()方法里，系统在处理链调度完最后
     * 一个节点处理完成时调用Done()方法将整个动态处理链的响应结果发给客户端，
     * 那样动态处理链的Block节点顺序只跟链的业务逻辑相关，而无需固定最后一个
     * 步骤。注意，该方法只适用于配置的动态处理链，预定义的静态处理链无需实
     * 现此方法，所以该方法是实现为空的方法而非纯虚方法。
     */
    virtual void Done()
    {
    }

    std::shared_ptr<SocketChannel> GetChannel()
    {
        return(m_pChannel);
    }

private:
    std::shared_ptr<SocketChannel> m_pChannel;
    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CONTEXT_HPP_ */
