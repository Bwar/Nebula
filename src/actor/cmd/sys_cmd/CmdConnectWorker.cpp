/*******************************************************************************
 * Project:  Nebula
 * @file     CmdConnectWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/cmd/sys_cmd/CmdConnectWorker.hpp>

namespace neb
{

CmdConnectWorker::CmdConnectWorker(int iCmd)
    : Cmd(iCmd)
{
}

CmdConnectWorker::~CmdConnectWorker()
{
}

bool CmdConnectWorker::AnyMessage(
                std::shared_ptr<SocketChannel> pChannel,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    MsgBody oOutMsgBody;
    oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
    oOutMsgBody.mutable_rsp_result()->set_msg("OK");
    return(SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody));
}

} /* namespace neb */
