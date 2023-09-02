/*******************************************************************************
 * Project:  Nebula
 * @file     ChannelWatcher.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-03
 * @note     
 * Modify history:
 ******************************************************************************/
#include "ChannelWatcher.hpp"
#include "channel/SocketChannel.hpp"

namespace neb
{

ChannelWatcher::ChannelWatcher()
    : m_pTimerWatcher(nullptr), m_pIoWatcher(nullptr),
      m_pSocketChannel(nullptr)
{
}

ChannelWatcher::ChannelWatcher(std::shared_ptr<SocketChannel> pChannel)
    : m_pTimerWatcher(nullptr), m_pIoWatcher(nullptr),
      m_pSocketChannel(pChannel)
{
}

ChannelWatcher::~ChannelWatcher()
{
    Reset();
}

ev_io* ChannelWatcher::MutableIoWatcher()
{
    if (nullptr == m_pSocketChannel)
    {
        return(nullptr);
    }
    if (nullptr == m_pIoWatcher)
    {
        m_pIoWatcher = new ev_io();
        if (nullptr != m_pIoWatcher)
        {
            memset(m_pIoWatcher, 0, sizeof(ev_io));
            m_pIoWatcher->data = this;
            m_pIoWatcher->fd = m_pSocketChannel->GetFd();
        }
    }
    return(m_pIoWatcher);
}

ev_timer* ChannelWatcher::MutableTimerWatcher()
{
    if (nullptr == m_pTimerWatcher)
    {
        m_pTimerWatcher = new ev_timer();
        if (nullptr != m_pTimerWatcher)
        {
            memset(m_pTimerWatcher, 0, sizeof(ev_timer));
            m_pTimerWatcher->data = this;
        }
    }
    return(m_pTimerWatcher);
}

void ChannelWatcher::Set(std::shared_ptr<SocketChannel> pChannel)
{
    if (m_pSocketChannel == nullptr)
    {
        m_pSocketChannel = pChannel;
    }
}

void ChannelWatcher::Reset()
{
    m_pSocketChannel = nullptr;
    if (nullptr != m_pTimerWatcher)
    {
        delete m_pTimerWatcher;
        m_pTimerWatcher = nullptr;
    }
    if (nullptr != m_pIoWatcher)
    {
        delete m_pIoWatcher;
        m_pIoWatcher = nullptr;
    }
}

} /* namespace neb */

