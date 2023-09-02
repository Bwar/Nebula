/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelPack.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-07
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_MIGRATE_SOCKETCHANNELPACK_HPP_
#define SRC_CHANNEL_MIGRATE_SOCKETCHANNELPACK_HPP_

#include "channel/SocketChannel.hpp"

namespace neb
{

class SocketChannelPack
{
public:
    SocketChannelPack();
    SocketChannelPack(const SocketChannelPack& oPack);
    SocketChannelPack(SocketChannelPack&& oPack);
    virtual ~SocketChannelPack();

    SocketChannelPack& operator=(const SocketChannelPack& oPack);
    SocketChannelPack& operator=(SocketChannelPack&& oPack);

    void PackChannel(std::shared_ptr<SocketChannel> pSocketChannel);
    std::shared_ptr<SocketChannel> UnpackChannel();

private:
    std::shared_ptr<SocketChannel> m_pSocketChannel = nullptr;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_MIGRATE_SOCKETCHANNELPACK_HPP_ */

