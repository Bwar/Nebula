/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelPack.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-07
 * @note     
 * Modify history:
 ******************************************************************************/
#include <channel/migrate/SocketChannelPack.hpp>

namespace neb
{

SocketChannelPack::SocketChannelPack()
{
}

SocketChannelPack::SocketChannelPack(const SocketChannelPack& oPack)
{
    m_pSocketChannel = oPack.m_pSocketChannel;
}

SocketChannelPack::SocketChannelPack(SocketChannelPack&& oPack)
{
    m_pSocketChannel = oPack.m_pSocketChannel;
    oPack.m_pSocketChannel = nullptr;
}

SocketChannelPack::~SocketChannelPack()
{
}

SocketChannelPack& SocketChannelPack::operator=(const SocketChannelPack& oPack)
{
    m_pSocketChannel = oPack.m_pSocketChannel;
    return(*this);
}

SocketChannelPack& SocketChannelPack::operator=(SocketChannelPack&& oPack)
{
    m_pSocketChannel = oPack.m_pSocketChannel;
    oPack.m_pSocketChannel = nullptr;
    return(*this);
}

void SocketChannelPack::PackChannel(std::shared_ptr<SocketChannel> pSocketChannel)
{
    m_pSocketChannel = pSocketChannel;
}

std::shared_ptr<SocketChannel> SocketChannelPack::UnpackChannel()
{
    return(m_pSocketChannel);
}

} /* namespace neb */

