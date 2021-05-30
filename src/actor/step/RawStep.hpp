/*******************************************************************************
 * Project:  Nebula
 * @file     RawStep.hpp
 * @brief 
 * @author   Bwar
 * @date:    2020年12月6日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_RAWSTEP_HPP_
#define SRC_ACTOR_STEP_RAWSTEP_HPP_

#include <set>
#include <list>
#include "actor/step/Step.hpp"
#include "pb/redis.pb.h"

namespace neb
{

class RawStep: public Step
{
public:
    RawStep(std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dConfigTimeout);
    RawStep(const RawStep&) = delete;
    RawStep& operator=(const RawStep&) = delete;
    virtual ~RawStep();

    /**
     * @brief 步骤回调
     * @note 回调的是裸数据，需要由RawStep派生类自行在Callback里进行解码（包括组包和拆包）。
     * 一次接收到uiRawDataSize大小的pRawData可能是一个完整的数据包，也可能只是部分数据包或
     * 多个数据包。用户自定义通信协议可以通过RawStep来实现收发和业务逻辑。
     * @param pChannel 消息来源通信通道
     * @param pRawData 数据指针
     * @param uiRawDataSize 数据长度
     */
    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const char* pRawData,
            uint32 uiRawDataSize) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_RAWSTEP_HPP_ */
