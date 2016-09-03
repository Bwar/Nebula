/*******************************************************************************
 * Project:  Nebula
 * @file     StepConnectWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_
#define SRC_OBJECT_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_

#include "object/step/PbStep.hpp"
#include "StepTellWorker.hpp"

namespace neb
{

class StepConnectWorker: public PbStep
{
public:
    StepConnectWorker(const tagChannelContext& stCtx, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    virtual ~StepConnectWorker();

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
        return("StepConnectWorker");
    }

public:
    StepTellWorker* pStepTellWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_ */
