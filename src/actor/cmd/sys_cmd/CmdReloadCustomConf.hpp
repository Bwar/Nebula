/*******************************************************************************
 * Project:  Nebula
 * @file     CmdReloadCustomConf.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDRELOADCUSTOMCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDRELOADCUSTOMCONF_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdReloadCustomConf: public Cmd,
    public DynamicCreator<CmdReloadCustomConf, int32>, public ActorSys
{
public:
    CmdReloadCustomConf(int32 iCmd);
    virtual ~CmdReloadCustomConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDRELOADCUSTOMCONF_HPP_ */
