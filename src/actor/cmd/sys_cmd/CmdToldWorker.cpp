/*******************************************************************************
 * Project:  Nebula
 * @file     CmdToldWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdToldWorker.hpp"
#include "pb/neb_sys.pb.h"
#include "ios/Dispatcher.hpp"

namespace neb
{

CmdToldWorker::CmdToldWorker(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdToldWorker::~CmdToldWorker()
{
}

bool CmdToldWorker::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    bool bResult = false;
    MsgBody oOutMsgBody;
    TargetWorker oInTargetWorker;
    TargetWorker oOutTargetWorker;
    if (oInTargetWorker.ParseFromString(oInMsgBody.data()))
    {
        bResult = true;
        LOG4_DEBUG("AddNodeIdentify(%s, %s)!", oInTargetWorker.node_type().c_str(),
                        oInTargetWorker.worker_identify().c_str());
// 发起连接的节点执行autosend时已添加过，这里已不再需要添加        GetWorkerImpl(this)->AddNamedSocketChannel(oInTargetWorker.worker_identify(), pChannel);
        GetLabor(this)->GetDispatcher()->AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
        oOutTargetWorker.set_worker_identify(GetNodeIdentify());
        oOutTargetWorker.set_node_type(GetNodeType());
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
        oOutMsgBody.mutable_rsp_result()->set_msg("OK");
    }
    else
    {
        bResult = false;
        oOutTargetWorker.set_worker_identify("unknow");
        oOutTargetWorker.set_node_type(GetNodeType());
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
        oOutMsgBody.mutable_rsp_result()->set_msg("WorkerLoad ParseFromString error!");
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
    }
    oOutMsgBody.set_data(oOutTargetWorker.SerializeAsString());
    SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
    return(bResult);
}


} /* namespace neb */
