/*******************************************************************************
 * Project:  Nebula
 * @file     CmdConnectWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_

#include "actor/cmd/Cmd.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

class CmdConnectWorker: public Cmd, public DynamicCreator<CmdConnectWorker, int>
{
public:
    CmdConnectWorker(int iCmd);
    virtual ~CmdConnectWorker();
    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_ */
