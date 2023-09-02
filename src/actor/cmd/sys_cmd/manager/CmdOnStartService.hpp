/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnStartService.hpp
 * @brief    启动服务
 * @author   Bwar
 * @date:    2020-07-05
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSTARTSERVICE_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSTARTSERVICE_HPP_

#include <unordered_set>
#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionManager;

class CmdOnStartService: public Cmd,
    public DynamicCreator<CmdOnStartService, int32>, public ActorSys
{
public:
    CmdOnStartService(int32 iCmd);
    virtual ~CmdOnStartService();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::unordered_set<uint32> m_setReadyWorker;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSTARTSERVICE_HPP_ */

