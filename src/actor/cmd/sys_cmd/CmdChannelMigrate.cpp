/*******************************************************************************
 * Project:  Nebula
 * @file     CmdChannelMigrate.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-08
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CmdChannelMigrate.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

CmdChannelMigrate::CmdChannelMigrate(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdChannelMigrate::~CmdChannelMigrate()
{
}

bool CmdChannelMigrate::AnyMessage(
            std::shared_ptr<SocketChannel> pChannel, SocketChannelPack& oPack)
{
    auto pMigratedChannel = oPack.UnpackChannel();
    GetLabor(this)->GetDispatcher()->AddChannelToLoop(pMigratedChannel);
    return(true);
}

} /* namespace neb */

