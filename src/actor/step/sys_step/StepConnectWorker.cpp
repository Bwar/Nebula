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

StepConnectWorker::StepConnectWorker(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
    : PbStep(pChannel, oInMsgHead, oInMsgBody),
      pStepTellWorker(nullptr)
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
    m_oReqMsgHead.set_seq(GetSequence());
    SendTo(m_pUpstreamChannel, m_oReqMsgHead.cmd(), m_oReqMsgHead.seq(), m_oReqMsgBody);
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
                pStepTellWorker =  std::dynamic_pointer_cast<StepTellWorker>(MakeSharedStep("neb::StepTellWorker", pChannel));
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
    LOG4_ERROR("timeout!");
    return(CMD_STATUS_FAULT);
}

} /* namespace neb */
