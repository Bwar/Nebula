/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnSetCustomConf.hpp
 * @brief    设置自定义配置文件
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSETCUSTOMCONF_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSETCUSTOMCONF_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionManager;

class CmdOnSetCustomConf: public Cmd,
    public DynamicCreator<CmdOnSetCustomConf, int32>, public ActorSys
{
public:
    CmdOnSetCustomConf(int32 iCmd);
    virtual ~CmdOnSetCustomConf();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);

private:
    std::shared_ptr<SessionManager> m_pSessionManager;
    std::string m_strDataString;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MANAGER_CMDONSETCUSTOMCONF_HPP_ */

