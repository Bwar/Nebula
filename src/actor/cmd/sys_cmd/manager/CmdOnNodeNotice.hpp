/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnNodeNotice.hpp
 * @brief    来自注册中心的节点通知
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONNODENOTICE_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONNODENOTICE_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionManager;

class CmdOnNodeNotice: public Cmd,
    public DynamicCreator<CmdOnNodeNotice, int32>,
    public ActorSys
{
public:
    CmdOnNodeNotice(int32 iCmd);
    virtual ~CmdOnNodeNotice();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::shared_ptr<SessionManager> m_pSessionManager;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONNODENOTICE_HPP_ */
