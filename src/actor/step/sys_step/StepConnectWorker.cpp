/*******************************************************************************
 * Project:  Nebula
 * @file     StepConnectWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/step/sys_step/StepConnectWorker.hpp>
#include "actor/cmd/CW.hpp"

namespace neb
{

StepConnectWorker::StepConnectWorker(std::shared_ptr<SocketChannel> pChannel, uint16 unRemoteWorkerIndex)
    : m_pChannel(pChannel), m_unRemoteWorkerIndex(unRemoteWorkerIndex)
{
}

StepConnectWorker::~StepConnectWorker()
{
}

E_CMD_STATUS StepConnectWorker::Emit(
        int iErrno,
        const std::string& strErrMsg,
        void* data)
{
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    oMsgBody.set_data(std::to_string((int)m_unRemoteWorkerIndex));
    SendTo(m_pChannel, CMD_REQ_CONNECT_TO_WORKER, GetSequence(), oMsgBody);
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepConnectWorker::Callback(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepConnectWorker::Timeout()
{
    LOG4_DEBUG(" ");
    return(CMD_STATUS_COMPLETED);
}

} /* namespace neb */
