/*******************************************************************************
 * Project:  Nebula
 * @file     StepTellWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_SYS_STEP_STEPTELLWORKER_HPP_
#define SRC_OBJECT_STEP_SYS_STEP_STEPTELLWORKER_HPP_

#include "pb/neb_sys.pb.h"
#include "object/step/PbStep.hpp"

namespace neb
{

class StepTellWorker: public PbStep
{
public:
    StepTellWorker(const tagChannelContext& stCtx);
    virtual ~StepTellWorker();

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
        return("StepTellWorker");
    }

private:
    tagChannelContext m_stCtx;
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_SYS_STEP_STEPTELLWORKER_HPP_ */
