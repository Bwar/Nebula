/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnTellWorker.hpp
 * @brief    告知Worker信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONTELLWORKER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONTELLWORKER_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdOnTellWorker: public Cmd,
    public DynamicCreator<CmdOnTellWorker, int32>, public ActorSys
{
public:
    CmdOnTellWorker(int32 iCmd);
    virtual ~CmdOnTellWorker();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::string m_strDataString;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONTELLWORKER_HPP_ */

