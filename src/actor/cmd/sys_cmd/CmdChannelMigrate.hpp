/*******************************************************************************
 * Project:  Nebula
 * @file     CmdChannelMigrate.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-08
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDCHANNELMIGRATE_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDCHANNELMIGRATE_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"
#include "channel/migrate/SocketChannelPack.hpp"

namespace neb
{

class CmdChannelMigrate: public Cmd,
    public DynamicCreator<CmdChannelMigrate, int32>, public ActorSys
{
public:
    CmdChannelMigrate(int32 iCmd);
    virtual ~CmdChannelMigrate();

    virtual bool Init()
    {
        return(true);
    }

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oMsgHead, const MsgBody& oMsgBody)
    {
        return(false);
    }

    bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            SocketChannelPack& oPack);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDCHANNELMIGRATE_HPP_ */

