/*******************************************************************************
 * Project:  Nebula
 * @file     CmdUpdateNodeId.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDUPDATENODEID_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDUPDATENODEID_HPP_

#include "actor/cmd/Cmd.hpp"
#include "labor/WorkerFriend.hpp"

namespace neb
{

class CmdUpdateNodeId: public Cmd, public DynamicCreator<CmdUpdateNodeId>, public WorkerFriend
{
public:
    CmdUpdateNodeId(int32 iCmd);
    virtual ~CmdUpdateNodeId();
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDUPDATENODEID_HPP_ */
