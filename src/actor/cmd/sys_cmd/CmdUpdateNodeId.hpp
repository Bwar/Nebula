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

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdUpdateNodeId: public Cmd,
    public DynamicCreator<CmdUpdateNodeId, int32>, public ActorSys
{
public:
    CmdUpdateNodeId(int32 iCmd);
    virtual ~CmdUpdateNodeId();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDUPDATENODEID_HPP_ */
