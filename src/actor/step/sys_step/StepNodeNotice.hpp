/*******************************************************************************
 * Project:  Nebula
 * @file     StepNodeNotice.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_SYS_STEP_STEPNODENOTICE_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_STEPNODENOTICE_HPP_

#include "labor/WorkerFriend.hpp"
#include "actor/step/PbStep.hpp"
#include "util/json/CJsonObject.hpp"

namespace neb
{

class StepNodeNotice: public PbStep, public DynamicCreator<StepNodeNotice, MsgBody>, public WorkerFriend
{
public:
    StepNodeNotice(const MsgBody& oMsgBody);
    virtual ~StepNodeNotice();

    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);

    virtual E_CMD_STATUS Callback(
            const tagChannelContext& stCtx,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual E_CMD_STATUS Timeout();

private:
    MsgBody m_oMsgBody;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPNODENOTICE_HPP_ */
