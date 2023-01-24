/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSpecChannelCreated.hpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-10
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDSPECCHANNELCREATED_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDSPECCHANNELCREATED_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdSpecChannelCreated: public Cmd,
    public DynamicCreator<CmdSpecChannelCreated, int32>, public ActorSys
{
public:
    CmdSpecChannelCreated(int32 iCmd);
    virtual ~CmdSpecChannelCreated();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDSPECCHANNELCREATED_HPP_ */

