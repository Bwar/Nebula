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

namespace neb
{

CmdToldWorker::CmdToldWorker()
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
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    TargetWorker oInTargetWorker;
    TargetWorker oOutTargetWorker;
    oOutMsgHead.set_cmd(oInMsgHead.cmd() + 1);
    oOutMsgHead.set_seq(oInMsgHead.seq());
    if (oInTargetWorker.ParseFromString(oInMsgBody.content()))
    {
        bResult = true;
        LOG4_DEBUG("AddMsgShell(%s, fd %d, seq %llu)!",
                        oInTargetWorker.worker_identify().c_str(), stCtx.iFd, stCtx.ulSeq);
        AddNamedChannel(oInTargetWorker.worker_identify(), stCtx);
        AddNodeIdentify(oInTargetWorker.node_type(), oInTargetWorker.worker_identify());
        AddInnerChannel(stCtx);
        oOutTargetWorker.set_worker_identify(GetWorkerIdentify());
        oOutTargetWorker.set_node_type(GetNodeType());
        oOutMsgBody.mutable_rsp_result()->set_err_no(ERR_OK);
        oOutMsgBody.mutable_rsp_result()->set_err_msg("OK");
    }
    else
    {
        bResult = false;
        oOutTargetWorker.set_worker_identify("unknow");
        oOutTargetWorker.set_node_type(GetNodeType());
        oOutMsgBody.mutable_rsp_result()->set_err_no(ERR_PARASE_PROTOBUF);
        oOutMsgBody.mutable_rsp_result()->set_err_msg("WorkerLoad ParseFromString error!");
        LOG4_ERROR("error %d: WorkerLoad ParseFromString error!", ERR_PARASE_PROTOBUF);
    }
    oOutMsgBody.set_content(oOutTargetWorker.SerializeAsString());
    oOutMsgHead.set_len(oOutMsgBody.ByteSize());
    SendTo(stCtx, oOutMsgHead, oOutMsgBody);
    return(bResult);
}


} /* namespace neb */
