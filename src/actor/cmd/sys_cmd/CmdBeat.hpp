/*******************************************************************************
 * Project:  Nebula
 * @file     CmdBeat.hpp
 * @brief    心跳包响应
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDBEAT_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDBEAT_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdBeat: public Cmd,
    public DynamicCreator<CmdBeat, int32>, public ActorSys
{
public:
    CmdBeat(int32 iCmd);
    virtual ~CmdBeat();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDBEAT_HPP_ */
