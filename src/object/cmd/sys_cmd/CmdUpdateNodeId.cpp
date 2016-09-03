/*******************************************************************************
 * Project:  Nebula
 * @file     CmdUpdateNodeId.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdUpdateNodeId.hpp"

namespace neb
{

CmdUpdateNodeId::CmdUpdateNodeId()
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
    loss::CJsonObject oNode;
    if (oNode.Parse(oInMsgBody.content()))
    {
        uint32 uiNodeId = 0;
        oNode.Get("node_id", uiNodeId);
        SetNodeId(uiNodeId);
        return(true);
    }
    return(false);
}

} /* namespace neb */
