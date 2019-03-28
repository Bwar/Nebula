/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetServerCustomConf.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCUSTOMCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCUSTOMCONF_HPP_

#include "actor/cmd/Cmd.hpp"
#include "labor/WorkerFriend.hpp"

namespace neb
{

class CmdSetServerCustomConf: public Cmd, public DynamicCreator<CmdSetServerCustomConf, int32>, public WorkerFriend
{
public:
    CmdSetServerCustomConf(int32 iCmd);
    virtual ~CmdSetServerCustomConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDSETSERVERCUSTOMCONF_HPP_ */
