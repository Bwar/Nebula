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
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepConnectWorker::Callback(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    if (oInMsgBody.has_rsp_result())
    {
        if (ERR_OK == oInMsgBody.rsp_result().code())
        {
                std::shared_ptr<Step> pStepTellWorker = MakeSharedStep("neb::StepTellWorker", pChannel);
                if (nullptr == pStepTellWorker)
                {
                    LOG4_ERROR("error %d: new StepTellWorker() error!", ERR_NEW);
                    return(CMD_STATUS_FAULT);
                }
                pStepTellWorker->Emit(ERR_OK);
                return(CMD_STATUS_COMPLETED);
        }
        else
        {
            LOG4_ERROR("error %d: %s!", oInMsgBody.rsp_result().code(), oInMsgBody.rsp_result().msg().c_str());
            return(CMD_STATUS_FAULT);
        }
    }
    else
    {
        LOG4_ERROR("error %d: oInMsgBody has no rsp_result!", ERR_PARASE_PROTOBUF);
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepConnectWorker::Timeout()
{
    LOG4_DEBUG(" ");
    return(CMD_STATUS_COMPLETED);
}

} /* namespace neb */
