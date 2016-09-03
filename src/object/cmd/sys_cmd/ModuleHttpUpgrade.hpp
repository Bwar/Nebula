/*******************************************************************************
 * Project:  Nebula
 * @file     CmdHttpUpgrade.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年9月1日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_

#include "codec/Codec.hpp"
#include "object/cmd/Module.hpp"

namespace neb
{

class ModuleHttpUpgrade: public Module
{
public:
    ModuleHttpUpgrade();
    virtual ~ModuleHttpUpgrade();

    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const HttpMsg& oHttpMsg);

protected:
    bool WebSocket(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg);

private:
    const std::string mc_strWebSocketMagicGuid;
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_MODULEHTTPUPGRADE_HPP_ */
