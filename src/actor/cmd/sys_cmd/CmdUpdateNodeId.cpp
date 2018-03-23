/*******************************************************************************
 * Project:  Nebula
 * @file     CmdUpdateNodeId.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdUpdateNodeId.hpp"

namespace neb
{

CmdUpdateNodeId::CmdUpdateNodeId(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdUpdateNodeId::~CmdUpdateNodeId()
{
}

bool CmdUpdateNodeId::AnyMessage(
        const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    CJsonObject oNode;
    if (oNode.Parse(oInMsgBody.data()))
    {
        uint32 uiNodeId = 0;
        oNode.Get("node_id", uiNodeId);
        SetNodeId(uiNodeId);
        return(true);
    }
    return(false);
}

} /* namespace neb */
