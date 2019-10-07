/*******************************************************************************
 * Project:  Nebula
 * @file     CmdHttpUpgrade.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年9月1日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_

#include "codec/Codec.hpp"
#include "actor/cmd/Module.hpp"
#include "actor/ActorFriend.hpp"

namespace neb
{

class ModuleHttpUpgrade: public Module,
    public DynamicCreator<ModuleHttpUpgrade, std::string>, public ActorFriend
{
public:
    ModuleHttpUpgrade(const std::string& strModulePath);
    virtual ~ModuleHttpUpgrade();

    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const HttpMsg& oHttpMsg);

protected:
    bool WebSocket(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);

private:
    const std::string mc_strWebSocketMagicGuid;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_ */
