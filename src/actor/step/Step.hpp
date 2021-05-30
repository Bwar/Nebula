/*******************************************************************************
 * Project:  Nebula
 * @file     Step.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_STEP_HPP_
#define SRC_ACTOR_STEP_STEP_HPP_

#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class ActorBuilder;
class Chain;

class Step: public Actor
{
public:
    Step(Actor::ACTOR_TYPE eActorType, std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dConfigTimeout);
    Step(const Step&) = delete;
    Step& operator=(const Step&) = delete;
    virtual ~Step();

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。
     */
    virtual E_CMD_STATUS Emit(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

    /**
     * @brief 错误回调
     * @note 由框架层回调，与业务逻辑无关。当框架明确网络错误发生且不可能产生多于
     * 一种结果时回调（比如异步网络发送错误）。为兼容之前版本，该方法设计为非纯虚
     * 方法。
     */
    virtual E_CMD_STATUS ErrBack(std::shared_ptr<SocketChannel> pChannel,
            int iErrno, const std::string& strErrMsg)
    {
        LOG4_ERROR("error %d: %s", iErrno, strErrMsg.c_str());
        return(CMD_STATUS_FAULT);
    }

    virtual E_CMD_STATUS Callback(std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oMsgHead, const MsgBody& oMsgBody, void* data = NULL);
    virtual E_CMD_STATUS Callback(std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oHttpMsg, void* data = NULL);
    virtual E_CMD_STATUS Callback(std::shared_ptr<SocketChannel> pChannel,
            const RedisReply& oRedisReply);
    virtual E_CMD_STATUS Callback(std::shared_ptr<SocketChannel> pChannel,
            const char* pRawData, uint32 uiRawDataSize);

protected:
    /**
     * @brief 执行当前步骤接下来的步骤
     */
    void NextStep(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);

    uint32 GetChainId() const
    {
        return(m_uiChainId);
    }

private:
    void SetChainId(uint32 uiChainId);

    uint32 m_uiChainId;
    std::unordered_set<uint32> m_setNextStepSeq;
    std::unordered_set<uint32> m_setPreStepSeq;

    friend class ActorBuilder;
    friend class Chain;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_STEP_HPP_ */
