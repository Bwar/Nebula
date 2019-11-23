/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnWorkerLoad.hpp
 * @brief    接收Worker节点负载等信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONWORKERLOAD_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONWORKERLOAD_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionManager;

class CmdOnWorkerLoad: public Cmd,
    public DynamicCreator<CmdOnWorkerLoad, int32>, public ActorSys
{
public:
    CmdOnWorkerLoad(int32 iCmd);
    virtual ~CmdOnWorkerLoad();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::shared_ptr<SessionManager> m_pSessionManager;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONWORKERLOAD_HPP_ */

