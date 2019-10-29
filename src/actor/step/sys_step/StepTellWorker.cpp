/*******************************************************************************
 * Project:  Nebula
 * @file     StepTellWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/step/sys_step/StepTellWorker.hpp>
#include "ios/Dispatcher.hpp"

namespace neb
{

StepTellWorker::StepTellWorker(std::shared_ptr<SocketChannel> pChannel)
    : m_pChannel(pChannel)
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
    MsgBody oOutMsgBody;
    TargetWorker oTargetWorker;
    oTargetWorker.set_worker_identify(GetNodeIdentify());
    oTargetWorker.set_node_type(GetNodeType());
    oOutMsgBody.set_data(oTargetWorker.SerializeAsString());
    Step::SendTo(m_pChannel, CMD_REQ_TELL_WORKER, GetSequence(), oOutMsgBody);
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepTellWorker::Callback(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody,
        void* data)
{
    if (ERR_OK == oInMsgBody.rsp_result().code())
    {
        TargetWorker oInTargetWorker;
        if (oInTargetWorker.ParseFromString(oInMsgBody.data()))
        {
            LOG4_DEBUG("AddNodeIdentify(%s)!", oInTargetWorker.worker_identify().c_str());
            GetLabor(this)->GetDispatcher()->AddNamedSocketChannel(oInTargetWorker.worker_identify(), pChannel);
            GetLabor(this)->GetDispatcher()->AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
            SendTo(pChannel);
            return(CMD_STATUS_COMPLETED);
        }
        else
        {
            LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
            return(CMD_STATUS_FAULT);
        }
    }
    else
    {
        LOG4_ERROR("error %d: %s!", oInMsgBody.rsp_result().code(), oInMsgBody.rsp_result().msg().c_str());
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepTellWorker::Timeout()
{
    LOG4_ERROR("timeout!");
    return(CMD_STATUS_FAULT);
}

} /* namespace neb */
