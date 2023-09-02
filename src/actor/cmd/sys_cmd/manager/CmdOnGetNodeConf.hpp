/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnGetNodeConf.hpp
 * @brief    获取节点配置
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETNODECONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETNODECONF_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdOnGetNodeConf: public Cmd,
    public DynamicCreator<CmdOnGetNodeConf, int32>, public ActorSys
{
public:
    CmdOnGetNodeConf(int32 iCmd);
    virtual ~CmdOnGetNodeConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::string m_strDataString;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETNODECONF_HPP_ */
