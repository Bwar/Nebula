/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnGetCustomConf.hpp
 * @brief    获取节点自定义配置
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETCUSTOMCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETCUSTOMCONF_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class CmdOnGetCustomConf: public Cmd,
    public DynamicCreator<CmdOnGetCustomConf, int32>, public ActorSys
{
public:
    CmdOnGetCustomConf(int32 iCmd);
    virtual ~CmdOnGetCustomConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::string m_strDataString;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONGETCUSTOMCONF_HPP_ */
