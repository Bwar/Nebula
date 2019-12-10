/*******************************************************************************
 * Project:  Nebula
 * @file     StepTellWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_SYS_STEP_STEPTELLWORKER_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_STEPTELLWORKER_HPP_

#include "actor/ActorSys.hpp"
#include "actor/step/PbStep.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

class StepTellWorker: public PbStep,
    public DynamicCreator<StepTellWorker, std::shared_ptr<SocketChannel> >,
    public ActorSys
{
public:
    StepTellWorker(std::shared_ptr<SocketChannel> pChannel);
    virtual ~StepTellWorker();

    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);

    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual E_CMD_STATUS Timeout();

private:
    std::shared_ptr<SocketChannel> m_pChannel;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPTELLWORKER_HPP_ */
