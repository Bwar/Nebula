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
        const tagChannelContext& stCtx,
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
        LOG_DEBUG("AddNodeIdentify(%s, %s, fd %d, seq %llu)!", oInTargetWorker.node_type().c_str(),
                        oInTargetWorker.worker_identify().c_str(), stCtx.iFd, stCtx.uiSeq);
        GetWorkerImpl(this)->AddNamedSocketChannel(oInTargetWorker.worker_identify(), stCtx);
        GetWorkerImpl(this)->AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
        GetWorkerImpl(this)->AddInnerChannel(stCtx);
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
        LOG_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
    }
    oOutMsgBody.set_data(oOutTargetWorker.SerializeAsString());
    SendTo(stCtx, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
    return(bResult);
}


} /* namespace neb */
