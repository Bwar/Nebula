/*******************************************************************************
 * Project:  Nebula
 * @file     CmdToldWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDTOLDWORKER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDTOLDWORKER_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdToldWorker: public Cmd,
    public DynamicCreator<CmdToldWorker, int32>, public ActorSys
{
public:
    CmdToldWorker(int32 iCmd);
    virtual ~CmdToldWorker();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDTOLDWORKER_HPP_ */
