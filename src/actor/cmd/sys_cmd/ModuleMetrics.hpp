/*******************************************************************************
 * Project:  Nebula
 * @file     ModuleMetrics.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-05-30
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_

#include "actor/cmd/Module.hpp"

namespace neb
{

class ModuleMetrics: public Module,
    public DynamicCreator<ModuleMetrics, std::string&>
{
public:
    ModuleMetrics(const std::string& strModulePath);
    virtual ~ModuleMetrics();

    virtual bool Init();
    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oHttpMsg);
private:
    std::string m_strApp;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_MODULEHEALTH_HPP_ */

