/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelMigrate.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-07
 * @note     
 * Modify history:
 ******************************************************************************/
#include "SocketChannelMigrate.hpp"

namespace neb
{

SocketChannelMigrate::SocketChannelMigrate()
{
}

SocketChannelMigrate::~SocketChannelMigrate()
{
}

bool SocketChannelMigrate::MigrateChannel(std::shared_ptr<SocketChannel> pSocketChannel, SocketChannelPack& oPack)
{
    auto pNewChannel = std::make_shared<SocketChannel>();
    pNewChannel->m_bIsClient = pSocketChannel->m_bIsClient;
    pNewChannel->m_bWithSsl = pSocketChannel->m_bWithSsl;
    pNewChannel->m_pImpl = pSocketChannel->m_pImpl;
    pSocketChannel->m_pImpl = nullptr;
    pNewChannel->m_pWatcher = pSocketChannel->m_pWatcher;
    pNewChannel->m_pWatcher->Reset();
    pSocketChannel->m_pWatcher = nullptr;
    pSocketChannel->SetMigrated(true);
    pNewChannel->SetBonding(nullptr, nullptr, nullptr);  // remove the current relationship before migration
    oPack.PackChannel(pNewChannel);
    return(true);
}

} /* namespace neb */

