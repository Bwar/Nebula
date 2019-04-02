/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetNodeConf.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月24日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECONF_HPP_

#include "actor/cmd/Cmd.hpp"
#include "labor/WorkerFriend.hpp"

namespace neb
{

class CmdSetNodeConf: public Cmd, public DynamicCreator<CmdSetNodeConf, int32>, public WorkerFriend
{
public:
    CmdSetNodeConf(int32 iCmd);
    virtual ~CmdSetNodeConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDSETNODECONF_HPP_ */
