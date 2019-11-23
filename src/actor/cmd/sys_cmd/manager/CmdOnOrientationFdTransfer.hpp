/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnOrientationFdTransfer.hpp
 * @brief    定向文件描述符传送
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONORIENTATIONFDTRANSFER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONORIENTATIONFDTRANSFER_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionManager;

class CmdOnOrientationFdTransfer: public Cmd,
        public DynamicCreator<CmdOnOrientationFdTransfer, int32>,
        public ActorSys
{
public:
    CmdOnOrientationFdTransfer(int32 iCmd);
    virtual ~CmdOnOrientationFdTransfer();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::shared_ptr<SessionManager> m_pSessionManager;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONORIENTATIONFDTRANSFER_HPP_ */

