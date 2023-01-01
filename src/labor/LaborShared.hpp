/*******************************************************************************
 * Project:  Nebula
 * @file     LaborShared.hpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-04
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_LABORSHARED_HPP_
#define SRC_LABOR_LABORSHARED_HPP_

#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include "Definition.hpp"
#include "ios/Dispatcher.hpp"
#include "channel/SocketChannel.hpp"

namespace neb
{

class SocketChannel;

template<typename T1, typename T2> class SpecChannel;

class LaborShared
{
public:
    // To improve performance use vector instead of map
    typedef std::vector<std::shared_ptr<SocketChannel>> T_VEC_CHANNEL_TO;
    typedef std::vector<T_VEC_CHANNEL_TO> T_VEC_CHANNEL_FROM;
    typedef std::vector<T_VEC_CHANNEL_FROM> T_VECCHANNEL_CODEC_TYPE;

    virtual ~LaborShared();

    static inline LaborShared* Instance(uint32 uiLaborNum = 2)
    {
        if (s_pInstance == nullptr)
        {
            s_pInstance = new LaborShared(uiLaborNum);
        }
        return(s_pInstance);
    }

    Dispatcher* GetDispatcher(uint32 uiLaborId);
    // AddDispatcher() if and only if the service starts
    void AddDispatcher(uint32 uiLaborId, Dispatcher* pDispatcher);
    std::shared_ptr<SocketChannel> GetSpecChannel(uint32 uiCodecType, uint32 uiFrom, uint32 uiTo);
    int AddSpecChannel(uint32 uiCodecType, uint32 uiFrom, uint32 uiTo, std::shared_ptr<SocketChannel> pChannel);
    std::shared_ptr<SpecChannel<MsgBody, MsgHead>> CreateInternalSpecChannel(uint32 uiFrom, uint32 uiTo);

    uint32 GetSpecChannelQueueSize() const
    {
        return(m_uiSpecChannelQueueSize);
    }

    void SetSpecChannelQueueSize(uint32 uiSize)
    {
        m_uiSpecChannelQueueSize = uiSize;
    }

private:
    explicit LaborShared(uint32 uiLaborNum);
    static LaborShared* s_pInstance;
    static std::mutex s_mutex;

private:
    uint32 m_uiLaborNum;
    uint32 m_uiSpecChannelQueueSize;
    std::atomic<uint32> m_uiCodecSize;
    std::vector<Dispatcher*> m_vecDispatcher;
    T_VECCHANNEL_CODEC_TYPE m_vecSpecChannel;
};

} /* namespace neb */

#endif /* SRC_LABOR_LABORSHARE_HPP_ */

