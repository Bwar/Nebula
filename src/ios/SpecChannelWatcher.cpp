/*******************************************************************************
 * Project:  Nebula
 * @file     SpecChannelWatcher.cpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-03
 * @note     
 * Modify history:
 ******************************************************************************/
#include "SpecChannelWatcher.hpp"

namespace neb
{

SpecChannelWatcher::SpecChannelWatcher()
    : m_uiSpecChannelCodecType(0), m_pAsyncWatcher(nullptr), m_pSpecChannel(nullptr)
{
}

SpecChannelWatcher::SpecChannelWatcher(std::shared_ptr<SocketChannel> pChannel)
    : m_uiSpecChannelCodecType(0), m_pAsyncWatcher(nullptr), m_pSpecChannel(pChannel)
{
}

SpecChannelWatcher::~SpecChannelWatcher()
{
    Reset();
}

ev_async* SpecChannelWatcher::MutableAsyncWatcher()
{
    if (nullptr == m_pSpecChannel)
    {
        return(nullptr);
    }
    if (nullptr == m_pAsyncWatcher)
    {
        m_pAsyncWatcher = new ev_async();
        if (nullptr != m_pAsyncWatcher)
        {
            memset(m_pAsyncWatcher, 0, sizeof(ev_async));
            m_pAsyncWatcher->data = this;
        }
    }
    return(m_pAsyncWatcher);
}

void SpecChannelWatcher::Set(std::shared_ptr<SocketChannel> pChannel, uint32 uiSpecChannelCodecType)
{
    if (m_pSpecChannel == nullptr)
    {
        m_pSpecChannel = pChannel;
    }
    m_uiSpecChannelCodecType = uiSpecChannelCodecType;
}

void SpecChannelWatcher::Reset()
{
    m_pSpecChannel = nullptr;
    if (nullptr != m_pAsyncWatcher)
    {
        delete m_pAsyncWatcher;
        m_pAsyncWatcher = nullptr;
    }
}

} /* namespace neb */

