/*******************************************************************************
 * Project:  Nebula
 * @file     StepTellWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepTellWorker.hpp"

namespace neb
{

StepTellWorker::StepTellWorker(const tagChannelContext& stCtx)
    : m_stCtx(stCtx)
{
}

StepTellWorker::~StepTellWorker()
{
}

E_CMD_STATUS StepTellWorker::Emit(
        int iErrno,
        const std::string& strErrMsg,
        void* data)
{
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    TargetWorker oTargetWorker;
    oTargetWorker.set_err_no(0);
    oTargetWorker.set_worker_identify(GetWorkerIdentify());
    oTargetWorker.set_node_type(GetNodeType());
    oTargetWorker.set_err_msg("OK");
    oOutMsgBody.set_content(oTargetWorker.SerializeAsString());
    oOutMsgHead.set_cmd(CMD_REQ_TELL_WORKER);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_len(oOutMsgBody.ByteSize());
    Step::SendTo(m_stCtx, oOutMsgHead, oOutMsgBody);
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepTellWorker::Callback(
        const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    TargetWorker oInTargetWorker;
    if (oInTargetWorker.ParseFromString(oInMsgBody.content()))
    {
        if (oInTargetWorker.err_no() == ERR_OK)
        {
            LOG4CPLUS_DEBUG_FMT(GetLogger(), "AddMsgShell(%s, fd %d, seq %llu)!",
                            oInTargetWorker.worker_identify().c_str(), stCtx.iFd, stCtx.ulSeq);
            AddNamedChannel(oInTargetWorker.worker_identify(), stCtx);
            AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
            SendTo(stCtx);
            return(CMD_STATUS_COMPLETED);
        }
        else
        {
            LOG4_ERROR("error %d: %s!", oInTargetWorker.err_no(), oInTargetWorker.err_msg().c_str());
            return(CMD_STATUS_FAULT);
        }
    }
    else
    {
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepTellWorker::Timeout()
{
    LOG4_ERROR("timeout!");
    return(CMD_STATUS_FAULT);
}

} /* namespace neb */
