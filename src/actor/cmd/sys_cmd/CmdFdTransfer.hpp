/*******************************************************************************
 * Project:  Nebula
 * @file     CmdFdTransfer.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-08
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDFDTRANSFER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDFDTRANSFER_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdFdTransfer: public Cmd,
    public DynamicCreator<CmdFdTransfer, int32>, public ActorSys
{
public:
    CmdFdTransfer(int32 iCmd);
    virtual ~CmdFdTransfer();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDFDTRANSFER_HPP_ */

