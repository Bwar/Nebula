/*******************************************************************************
 * Project:  Nebula
 * @file     DeliverStep.hpp
 * @brief 
 * @author   Bwar
 * @date:    2023-02-11
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_DELIVERSTEP_HPP_
#define SRC_ACTOR_STEP_DELIVERSTEP_HPP_

#include "Step.hpp"
#include "type/Package.hpp"

namespace neb
{

class DeliverStep: public Step
{
public:
    DeliverStep(ev_tstamp dTimeout = gc_dConfigTimeout);
    DeliverStep(const DeliverStep&) = delete;
    DeliverStep& operator=(const DeliverStep&) = delete;
    virtual ~DeliverStep();

    /**
     * @brief redis步骤回调
     * @param pChannel 数据来源通道
     * @param oRedisReply redis响应
     */
    virtual E_CMD_STATUS Callback(
                    std::shared_ptr<SocketChannel> pChannel,
                    Package&& oPackage) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_DELIVERSTEP_HPP_ */

