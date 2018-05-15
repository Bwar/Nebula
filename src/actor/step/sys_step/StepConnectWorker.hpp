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

class StepConnectWorker: public PbStep, public DynamicCreator<StepConnectWorker, std::shared_ptr<SocketChannel>, uint16>
{
public:
    StepConnectWorker(std::shared_ptr<SocketChannel> pChannel, uint16 unRemoteWorkerIndex);
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

private:
    std::shared_ptr<SocketChannel> m_pChannel;
    uint16 m_unRemoteWorkerIndex;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPCONNECTWORKER_HPP_ */
