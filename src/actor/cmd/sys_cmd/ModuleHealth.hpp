/*******************************************************************************
 * Project:  Nebula
 * @file     ModuleHealth.hpp
 * @brief
 * @author   Bwar
 * @date:    2020-2-16
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_

#include "actor/cmd/Module.hpp"

namespace neb
{

class ModuleHealth: public Module,
    public DynamicCreator<ModuleHealth, std::string&>
{
public:
    ModuleHealth(const std::string& strModulePath);
    virtual ~ModuleHealth();

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oHttpMsg);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_ */

