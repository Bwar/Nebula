/*******************************************************************************
 * Project:  Nebula
 * @file     ChannelWatcher.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_CHANNELWATCHER_HPP_
#define SRC_IOS_CHANNELWATCHER_HPP_

#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include "ev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

namespace neb
{

class SocketChannel;

enum class ACTOR_CB_TYPE
{
    ACTOR_CB_NONE           = 0x00,     ///< 未定义回调
    ACTOR_CB_TIMER          = 0x01,     ///< 定时器回调
    ACTOR_CB_IO_SEQ         = 0x02,     ///< IO seq回调
    ACTOR_CB_IO_POINTER     = 0x04,     ///< IO 指针回调
};

class ChannelWatcher
{
public:
    ChannelWatcher();
    ChannelWatcher(std::shared_ptr<SocketChannel> pChannel);
    virtual ~ChannelWatcher();

    ev_io* MutableIoWatcher();
    ev_timer* MutableTimerWatcher();

    inline std::shared_ptr<SocketChannel> GetSocketChannel() const
    {
        return(m_pSocketChannel);
    }

    void Set(std::shared_ptr<SocketChannel> pChannel);
    void Reset();

private:
    ev_timer* m_pTimerWatcher;
    ev_io* m_pIoWatcher;
    std::shared_ptr<SocketChannel> m_pSocketChannel;
};

} /* namespace neb */

#endif /* SRC_IOS_CHANNELWATCHER_HPP_ */

