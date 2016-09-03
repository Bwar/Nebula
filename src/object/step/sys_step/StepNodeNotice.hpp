/*******************************************************************************
 * Project:  Nebula
 * @file     StepNodeNotice.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_SYS_STEP_STEPNODENOTICE_HPP_
#define SRC_OBJECT_STEP_SYS_STEP_STEPNODENOTICE_HPP_

#include "util/json/CJsonObject.hpp"
#include "object/step/PbStep.hpp"

namespace neb
{

class StepNodeNotice: public PbStep
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

    virtual std::string ObjectName() const
    {
        return("StepNodeNotice");
    }

private:
    MsgBody m_oMsgBody;
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_SYS_STEP_STEPNODENOTICE_HPP_ */
