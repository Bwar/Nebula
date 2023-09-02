/*******************************************************************************
 * Project:  Nebula
 * @file     CmdBeat.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdBeat.hpp"

namespace neb
{

CmdBeat::CmdBeat(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdBeat::~CmdBeat()
{
}

bool CmdBeat::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    bool bResult = false;
    MsgBody oOutMsgBody = oInMsgBody;
    SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
    return(bResult);
}

} /* namespace neb */
