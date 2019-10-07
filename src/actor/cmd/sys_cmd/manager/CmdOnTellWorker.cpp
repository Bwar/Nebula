/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnTellWorker.cpp
 * @brief    告知Worker信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnTellWorker.hpp"
#include "labor/NodeInfo.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

CmdOnTellWorker::CmdOnTellWorker(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnTellWorker::~CmdOnTellWorker()
{
}

bool CmdOnTellWorker::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    MsgBody oOutMsgBody;
    TargetWorker oInTargetWorker;
    TargetWorker oOutTargetWorker;
    if (oInTargetWorker.ParseFromString(oInMsgBody.data()))
    {
        LOG4_DEBUG("AddNodeIdentify(%s, %s)!", oInTargetWorker.node_type().c_str(),
                 oInTargetWorker.worker_identify().c_str());
        GetLabor(this)->GetDispatcher()->AddNamedSocketChannel(oInTargetWorker.worker_identify(), pChannel);
        GetLabor(this)->GetDispatcher()->AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
        oOutTargetWorker.set_worker_identify(GetNodeIdentify());
        oOutTargetWorker.set_node_type(GetLabor(this)->GetNodeInfo().strNodeType);
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
        oOutMsgBody.mutable_rsp_result()->set_msg("OK");
        //oOutMsgBody.set_data(oOutTargetWorker.SerializeAsString());
        oOutTargetWorker.SerializeToString(&m_strDataString);
        oOutMsgBody.set_data(m_strDataString);
        return(SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody));
    }
    else
    {
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
        oOutTargetWorker.set_worker_identify("unknow");
        oOutTargetWorker.set_node_type(GetLabor(this)->GetNodeInfo().strNodeType);
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
        oOutMsgBody.mutable_rsp_result()->set_msg("WorkerLoad ParseFromString error!");
        return(SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody));
    }
}

} /* namespace neb */
