/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetServerConf.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月24日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCONF_HPP_

#include "actor/cmd/Cmd.hpp"
#include "labor/WorkerFriend.hpp"

namespace neb
{

class CmdSetServerConf: public Cmd, public DynamicCreator<CmdSetServerConf, int32>, public WorkerFriend
{
public:
    CmdSetServerConf(int32 iCmd);
    virtual ~CmdSetServerConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCONF_HPP_ */
