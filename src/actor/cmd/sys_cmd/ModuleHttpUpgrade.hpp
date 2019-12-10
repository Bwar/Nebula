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

#include "actor/ActorSys.hpp"
#include "codec/Codec.hpp"
#include "actor/cmd/Module.hpp"

namespace neb
{

class ModuleHttpUpgrade: public Module,
    public DynamicCreator<ModuleHttpUpgrade, std::string>, public ActorSys
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
