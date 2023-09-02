/*******************************************************************************
 * Project:  Nebula
 * @file     StepRedisChannelPing.hpp
 * @brief 
 * @author   Bwar
 * @date:    2023-01-24
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_SYS_STEP_STEPREDISCHANNELBEAT_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_STEPREDISCHANNELBEAT_HPP_

#include "actor/step/RedisStep.hpp"
#include "actor/ActorSys.hpp"

namespace neb
{

class StepRedisChannelPing: public RedisStep,
    public DynamicCreator<StepRedisChannelPing, std::shared_ptr<SocketChannel>& >,
    public ActorSys
{
public:
    StepRedisChannelPing(std::shared_ptr<SocketChannel> pChannel);
    virtual ~StepRedisChannelPing();

    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);


    virtual E_CMD_STATUS Callback(
                    std::shared_ptr<SocketChannel> pChannel,
                    const RedisReply& oRedisReply);

    virtual E_CMD_STATUS Timeout();

private:
    std::shared_ptr<SocketChannel> m_pChannel;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPREDISCHANNELBEAT_HPP_ */

