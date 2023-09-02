/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetNodeCustomConf.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECUSTOMCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECUSTOMCONF_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdSetNodeCustomConf: public Cmd,
    public DynamicCreator<CmdSetNodeCustomConf, int32>, public ActorSys
{
public:
    CmdSetNodeCustomConf(int32 iCmd);
    virtual ~CmdSetNodeCustomConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECUSTOMCONF_HPP_ */
