/*******************************************************************************
 * Project:  Nebula
 * @file     SpecChannel.hpp
 * @brief    lock-free/wait-free spec channel
 * @author   Bwar
 * @date:    2022-12-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SPECCHANNEL_HPP_
#define SRC_CHANNEL_SPECCHANNEL_HPP_

#include <vector>
#include <atomic>
#include "SocketChannel.hpp"
#include "ios/SpecChannelWatcher.hpp"

namespace neb
{

template<typename Tdata, typename Thead = int>
class SpecChannel: public SocketChannel
{
public:
    SpecChannel(uint32 uiWriteWorker, uint32 uiReadWorker,
            uint32 uiQueueSize, bool bWithHeader = false);
    virtual ~SpecChannel();

    int Write(uint32 uiFlags, uint32 uiStepSeq, Tdata&& oData);

    int Write(uint32 uiFlags, uint32 uiStepSeq, Thead&& oHead, Tdata&& oData);

    bool Read(uint32& uiFlags, uint32& uiStepSeq, Tdata& oData);

    bool Read(uint32& uiFlags, uint32& uiStepSeq, Thead& oHead, Tdata& oData);

    bool IsEmpty() const;

    void GetEnds(uint32& uiFrom, uint32& uiTo) const;

    virtual int GetFd() const override
    {
        return(0);
    }

    virtual uint32 GetSequence() const override
    {
        return(0);
    }

    virtual bool IsClient() const override
    {
        return(false);
    }

    virtual bool IsPipeline() const override
    {
        return(true);
    }

    virtual const std::string& GetIdentify() const override
    {
        return(m_strIdentify);
    }

    virtual const std::string& GetRemoteAddr() const override
    {
        return(m_strRemoteAddr);
    }

    virtual E_CODEC_TYPE GetCodecType() const override
    {
        return(CODEC_TRANSFER);
    }

    virtual uint32 GetPeerStepSeq() const
    {
        return(m_uiPeerStepSeq);
    }

    uint32 GetOwnerId() const
    {
        return(m_uiReadLaborIndex);
    }

    SpecChannelWatcher* MutableWatcher();

protected:
    virtual void SetIdentify(const std::string& strIdentify);
    virtual void SetRemoteAddr(const std::string& strRemoteAddr);
    void WriteData(Tdata&& oData);
    void WriteHeadAndData(Thead&& oHead, Tdata&& oData);

private:
    bool m_bWithHeader;
    uint32 m_uiWriteLaborIndex;
    uint32 m_uiReadLaborIndex;
    uint32 m_uiSize;
    uint32 m_uiEffectiveSize;
    uint32 m_uiPeerStepSeq;
    std::atomic<uint32> m_uiWriteIndex;
    std::atomic<uint32> m_uiReadIndex;
    std::string m_strIdentify;
    std::string m_strRemoteAddr;
    std::vector<uint32> m_vecFlags;
    std::vector<std::pair<uint32, Tdata>> m_vecData;
    std::vector<Thead> m_vecHead;
    SpecChannelWatcher* m_pWatcher;
};

template<typename Tdata, typename Thead>
SpecChannel<Tdata, Thead>::SpecChannel(
        uint32 uiWriteWorker, uint32 uiReadWorker, uint32 uiQueueSize, bool bWithHeader)
    : m_bWithHeader(bWithHeader),
      m_uiWriteLaborIndex(uiWriteWorker), m_uiReadLaborIndex(uiReadWorker),
      m_uiSize(uiQueueSize), m_uiEffectiveSize(0), m_uiPeerStepSeq(0),
      m_uiWriteIndex(0), m_uiReadIndex(0),
      m_pWatcher(nullptr)
{
}

template<typename Tdata, typename Thead>
SpecChannel<Tdata, Thead>::~SpecChannel()
{
    if (nullptr != m_pWatcher)
    {
        delete m_pWatcher;
        m_pWatcher = nullptr;
    }
}

template<typename Tdata, typename Thead>
int SpecChannel<Tdata, Thead>::Write(uint32 uiFlags, uint32 uiStepSeq, Tdata&& oData)
{
    auto const uiCurrentWrite = m_uiWriteIndex.load(std::memory_order_relaxed);
    auto uiNextRecord = uiCurrentWrite + 1;
    uiNextRecord = (uiNextRecord == m_uiSize) ? 0 : uiNextRecord;
    if (m_uiEffectiveSize < m_uiSize)
    {
        m_vecFlags.push_back(uiFlags);
        m_vecData.emplace_back(std::move(std::make_pair(uiStepSeq, std::forward<Tdata>(oData))));
        m_uiWriteIndex.store(uiNextRecord, std::memory_order_release);
        ++m_uiEffectiveSize;
        return(ERR_OK);
    }
    else
    {
        if (uiNextRecord == m_uiReadIndex.load(std::memory_order_acquire))  // queue is full
        {
            return(ERR_SPEC_CHANNEL_FULL);
        }
        else
        {
            m_vecFlags[uiCurrentWrite] = uiFlags;
            m_vecData[uiCurrentWrite].first = uiStepSeq;
            m_vecData[uiCurrentWrite].second = std::forward<Tdata>(oData);
            m_uiWriteIndex.store(uiNextRecord, std::memory_order_release);
            return(ERR_OK);
        }
    }
}

template<typename Tdata, typename Thead>
int SpecChannel<Tdata, Thead>::Write(uint32 uiFlags, uint32 uiStepSeq, Thead&& oHead, Tdata&& oData)
{
    auto const uiCurrentWrite = m_uiWriteIndex.load(std::memory_order_relaxed);
    auto uiNextRecord = uiCurrentWrite + 1;
    uiNextRecord = (uiNextRecord == m_uiSize) ? 0 : uiNextRecord;
    if (m_uiEffectiveSize < m_uiSize)
    {
        m_vecFlags.push_back(uiFlags);
        m_vecData.emplace_back(std::move(std::make_pair(uiStepSeq, std::forward<Tdata>(oData))));
        m_vecHead.emplace_back(std::forward<Thead>(oHead));
        m_uiWriteIndex.store(uiNextRecord, std::memory_order_release);
        ++m_uiEffectiveSize;
        return(ERR_OK);
    }
    else
    {
        if (uiNextRecord == m_uiReadIndex.load(std::memory_order_acquire))  // queue is full
        {
            return(ERR_SPEC_CHANNEL_FULL);
        }
        else
        {
            m_vecFlags[uiCurrentWrite] = uiFlags;
            m_vecData[uiCurrentWrite].first = uiStepSeq;
            m_vecData[uiCurrentWrite].second = std::forward<Tdata>(oData);
            m_vecHead[uiCurrentWrite] = std::forward<Thead>(oHead);
            m_uiWriteIndex.store(uiNextRecord, std::memory_order_release);
            return(ERR_OK);
        }
    }
}

template<typename Tdata, typename Thead>
bool SpecChannel<Tdata, Thead>::Read(uint32& uiFlags, uint32& uiStepSeq, Tdata& oData)
{
    auto const uiCurrentRead = m_uiReadIndex.load(std::memory_order_relaxed);
    if (uiCurrentRead == m_uiWriteIndex.load(std::memory_order_acquire)) // queue is empty
    {
        return(false);
    }

    auto uiNextRecord = uiCurrentRead + 1;
    uiNextRecord = (uiNextRecord == m_uiSize) ? 0 : uiNextRecord;
    uiFlags = m_vecFlags[uiCurrentRead];
    uiStepSeq = m_vecData[uiCurrentRead].first;
    oData = std::move(m_vecData[uiCurrentRead].second);
    m_uiPeerStepSeq = uiStepSeq;
    m_uiReadIndex.store(uiNextRecord, std::memory_order_release);
    return(true);
}

template<typename Tdata, typename Thead>
bool SpecChannel<Tdata, Thead>::Read(uint32& uiFlags, uint32& uiStepSeq, Thead& oHead, Tdata& oData)
{
    auto const uiCurrentRead = m_uiReadIndex.load(std::memory_order_relaxed);
    if (uiCurrentRead == m_uiWriteIndex.load(std::memory_order_acquire)) // queue is empty
    {
        return(false);
    }

    auto uiNextRecord = uiCurrentRead + 1;
    uiNextRecord = (uiNextRecord == m_uiSize) ? 0 : uiNextRecord;
    uiFlags = m_vecFlags[uiCurrentRead];
    uiStepSeq = m_vecData[uiCurrentRead].first;
    oData = std::move(m_vecData[uiCurrentRead].second);
    oHead = std::move(m_vecHead[uiCurrentRead]);
    m_uiPeerStepSeq = uiStepSeq;
    m_uiReadIndex.store(uiNextRecord, std::memory_order_release);
    return(true);
}

template<typename Tdata, typename Thead>
bool SpecChannel<Tdata, Thead>::IsEmpty() const
{
    return(m_uiReadIndex.load(std::memory_order_acquire)
            == m_uiWriteIndex.load(std::memory_order_acquire));
}

template<typename Tdata, typename Thead>
void SpecChannel<Tdata, Thead>::GetEnds(uint32& uiFrom, uint32& uiTo) const
{
    uiFrom = m_uiWriteLaborIndex;
    uiTo = m_uiReadLaborIndex;
}

template<typename Tdata, typename Thead>
void SpecChannel<Tdata, Thead>::SetIdentify(const std::string& strIdentify)
{
    m_strIdentify = strIdentify;
}

template<typename Tdata, typename Thead>
void SpecChannel<Tdata, Thead>::SetRemoteAddr(const std::string& strRemoteAddr)
{
    m_strRemoteAddr = strRemoteAddr;
}

template<typename Tdata, typename Thead>
SpecChannelWatcher* SpecChannel<Tdata, Thead>::MutableWatcher()
{
    if (nullptr == m_pWatcher)
    {
        try
        {
            m_pWatcher = new SpecChannelWatcher();
        }
        catch(std::bad_alloc& e)
        {
            LOG4_TRACE("new AsyncWatcher error %s", e.what());
        }
    }
    return(m_pWatcher);
}

} /* namespace neb */

#endif /* SRC_CHANNEL_SPECCHANNEL_HPP_ */

