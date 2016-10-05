/*******************************************************************************
 * Project:  Nebula
 * @file     CmdBeat.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdBeat.hpp"

namespace neb
{

CmdBeat::CmdBeat()
{
}

CmdBeat::~CmdBeat()
{
}

bool CmdBeat::AnyMessage(
        const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    bool bResult = false;
    MsgBody oOutMsgBody = oInMsgBody;
    SendTo(stCtx, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
    return(bResult);
}

} /* namespace neb */
