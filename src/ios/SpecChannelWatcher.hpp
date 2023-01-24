/*******************************************************************************
 * Project:  Nebula
 * @file     SpecChannelWatcher.hpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_SPECCHANNELWATCHER_HPP_
#define SRC_IOS_SPECCHANNELWATCHER_HPP_

#include <memory>
#include "Definition.hpp"

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

class SpecChannelWatcher
{
public:
    SpecChannelWatcher();
    SpecChannelWatcher(std::shared_ptr<SocketChannel> pChannel);
    virtual ~SpecChannelWatcher();

    ev_async* MutableAsyncWatcher();

    inline std::shared_ptr<SocketChannel> GetSocketChannel() const
    {
        return(m_pSpecChannel);
    }

    uint32 GetCodecType() const
    {
        return(m_uiSpecChannelCodecType);
    }

    void Set(std::shared_ptr<SocketChannel> pChannel, uint32 uiSpecChannelCodecType);
    void Reset();

private:
    uint32 m_uiSpecChannelCodecType;
    ev_async* m_pAsyncWatcher;
    std::shared_ptr<SocketChannel> m_pSpecChannel;
};

} /* namespace neb */

#endif /* SRC_IOS_SPECCHANNELWATCHER_HPP_ */


