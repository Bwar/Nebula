/*******************************************************************************
 * Project:  Nebula
 * @file     StepConnectWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepConnectWorker.hpp"

namespace neb
{

StepConnectWorker::StepConnectWorker(const tagChannelContext& stCtx, const MsgHead& oInMsgHead, const MsgBody& oInMsgBody)
    : PbStep(stCtx, oInMsgHead, oInMsgBody),
      pStepTellWorker(NULL)
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
    SendTo(m_stCtx, m_oReqMsgHead.cmd(), m_oReqMsgHead.seq(), m_oReqMsgBody);
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepConnectWorker::Callback(
        const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    if (oInMsgBody.has_rsp_result())
    {
        if (ERR_OK == oInMsgBody.rsp_result().code())
        {
            for (int i = 0; i < 3; ++i)
            {
                pStepTellWorker = new StepTellWorker(stCtx);
                if (pStepTellWorker == NULL)
                {
                    LOG4_ERROR("error %d: new StepTellWorker() error!", ERR_NEW);
                    return(CMD_STATUS_FAULT);
                }

                if (Register(pStepTellWorker))
                {
                    pStepTellWorker->Emit(ERR_OK);
                    return(CMD_STATUS_COMPLETED);
                }
                else
                {
                    delete pStepTellWorker;
                }
            }
            return(CMD_STATUS_FAULT);
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
