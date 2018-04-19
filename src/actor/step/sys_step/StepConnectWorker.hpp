/*******************************************************************************
 * Project:  Nebula
 * @file     StepConnectWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_

#include <actor/step/PbStep.hpp>
#include <actor/step/sys_step/StepTellWorker.hpp>

namespace neb
{

class StepConnectWorker: public PbStep, public DynamicCreator<StepConnectWorker, std::shared_ptr<SocketChannel>, MsgHead, MsgBody>
{
public:
    StepConnectWorker(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody);
    virtual ~StepConnectWorker();

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

public:
    std::shared_ptr<StepTellWorker> pStepTellWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_ */
