/*******************************************************************************
 * Project:  Nebula
 * @file     LaborShared.cpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-04
 * @note     
 * Modify history:
 ******************************************************************************/
#include "LaborShared.hpp"
#include <algorithm>
#include "pb/neb_sys.pb.h"
#include "codec/CodecProto.hpp"
#include "channel/SpecChannel.hpp"

namespace neb
{

LaborShared* LaborShared::s_pInstance = nullptr;
std::mutex LaborShared::s_mutex;

LaborShared::LaborShared(uint32 uiLaborNum)
    : m_uiLaborNum(uiLaborNum), m_uiSpecChannelQueueSize(128), m_uiCodecSize(0)
{
    m_vecDispatcher.reserve(uiLaborNum);
    m_vecSpecChannel.reserve(CODEC_MAX + 1);
}

LaborShared::~LaborShared()
{
}

Dispatcher* LaborShared::GetDispatcher(uint32 uiLaborId)
{
    if (uiLaborId < m_vecDispatcher.size())
    {
        return(m_vecDispatcher[uiLaborId]);
    }
    else
    {
        return(nullptr);
    }
}

void LaborShared::AddDispatcher(uint32 uiLaborId, Dispatcher* pDispatcher)
{
    if (uiLaborId >= m_uiLaborNum)
    {
        return;
    }
    if (uiLaborId < m_vecDispatcher.size())
    {
        m_vecDispatcher[uiLaborId] = pDispatcher;
    }
    else
    {
        std::lock_guard<std::mutex> guard(s_mutex);
        for (uint32 i = m_vecDispatcher.size(); i <= uiLaborId; ++i)
        {
            m_vecDispatcher.push_back(nullptr);
        }
        m_vecDispatcher[uiLaborId] = pDispatcher;
    }
}

std::shared_ptr<SocketChannel> LaborShared::GetSpecChannel(uint32 uiCodecType, uint32 uiFrom, uint32 uiTo)
{
    if (uiFrom >= m_uiLaborNum || uiTo >= m_uiLaborNum)
    {
        return(nullptr);
    }
    if (uiCodecType < m_uiCodecSize.load(std::memory_order_relaxed))
    {
        return(m_vecSpecChannel[uiCodecType][uiFrom][uiTo]);
    }
    else
    {
        return(nullptr);
    }
}

int LaborShared::AddSpecChannel(uint32 uiCodecType, uint32 uiFrom, uint32 uiTo, std::shared_ptr<SocketChannel> pChannel)
{
    if (uiFrom >= m_uiLaborNum || uiTo >= m_uiLaborNum)
    {
        return(ERR_SPEC_CHANNEL_LABOR_ID);
    }
    if (uiCodecType < m_vecSpecChannel.size())
    {
        m_vecSpecChannel[uiCodecType][uiFrom][uiTo] = pChannel;
    }
    else
    {
        std::lock_guard<std::mutex> guard(s_mutex);
        for (uint32 i = m_vecSpecChannel.size(); i <= uiCodecType; ++i)
        {
            T_VEC_CHANNEL_FROM vecFrom;
            vecFrom.reserve(m_uiLaborNum);
            for (uint32 j = 0; j < m_uiLaborNum; ++j)
            {
                auto pTo = std::make_shared<T_VEC_CHANNEL_TO>();
                T_VEC_CHANNEL_TO vecTo;
                vecTo.resize(m_uiLaborNum, nullptr);
                vecFrom.emplace_back(std::move(vecTo));
            }
            m_vecSpecChannel.emplace_back(std::move(vecFrom));
        }
        m_vecSpecChannel[uiCodecType][uiFrom][uiTo] = pChannel;
        m_uiCodecSize.store(m_vecSpecChannel.size(), std::memory_order_release);
    }

    // notice spec channel created
    SpecChannelInfo oSpecInfo;
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    std::string strSpecInfo;
    oSpecInfo.set_codec_type(uiCodecType);
    oSpecInfo.set_from_labor(uiFrom);
    oSpecInfo.set_to_labor(uiTo);
    oSpecInfo.SerializeToString(&strSpecInfo);
    oMsgBody.set_data(strSpecInfo);
    oMsgHead.set_cmd(CMD_REQ_SPEC_CHANNEL);
    std::shared_ptr<SpecChannel<MsgBody, MsgHead>> pNoticeSpecChannel = nullptr;
    uint32 uiNoticeLabor = 0;
    if (uiFrom == m_uiLaborNum - 1) // manager
    {
        uiNoticeLabor = uiTo;
        auto pChannel = GetSpecChannel(CodecNebulaInNode::Type(), uiFrom, uiNoticeLabor);
        if (pChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_MANAGER);
        }
        else
        {
            pNoticeSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
        }
    }
    else
    {
        uiNoticeLabor = m_uiLaborNum - 1;   // manager labor id
        auto pChannel = GetSpecChannel(CodecNebulaInNode::Type(), uiFrom, uiNoticeLabor);    // forward notifications through manager
        if (pChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_MANAGER);
        }
        else
        {
            pNoticeSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
        }
    }
    if (pNoticeSpecChannel == nullptr)
    {
        return(ERR_SPEC_CHANNEL_CAST);
    }
    int iResult = pNoticeSpecChannel->Write(gc_uiCmdReq, 0, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody));
    if (iResult == ERR_OK)
    {
        GetDispatcher(uiNoticeLabor)->AsyncSend(pNoticeSpecChannel->MutableWatcher()->MutableAsyncWatcher());
    }
    return(iResult);
}

std::shared_ptr<SpecChannel<MsgBody, MsgHead>> LaborShared::CreateInternalSpecChannel(uint32 uiFrom, uint32 uiTo)
{
    uint32 uiCodecType = CodecNebulaInNode::Type();
    if (uiFrom >= m_uiLaborNum || uiTo >= m_uiLaborNum)
    {
        return(nullptr);
    }
    auto pSpecChannel = std::make_shared<SpecChannel<MsgBody, MsgHead>>(
            uiFrom, uiTo, m_uiSpecChannelQueueSize, true);
    if (pSpecChannel == nullptr)
    {
        return(nullptr);
    }
    auto pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
    auto pWatcher = pSpecChannel->MutableWatcher();
    pWatcher->Set(pChannel, uiCodecType);
    if (uiCodecType < m_vecSpecChannel.size())
    {
        m_vecSpecChannel[uiCodecType][uiFrom][uiTo] = pChannel;
    }
    else
    {
        std::lock_guard<std::mutex> guard(s_mutex);
        for (uint32 i = m_vecSpecChannel.size(); i <= uiCodecType; ++i)
        {
            T_VEC_CHANNEL_FROM vecFrom;
            for (uint32 j = 0; j < m_uiLaborNum; ++j)
            {
                auto pTo = std::make_shared<T_VEC_CHANNEL_TO>();
                T_VEC_CHANNEL_TO vecTo;
                vecTo.resize(m_uiLaborNum, nullptr);
                vecFrom.emplace_back(std::move(vecTo));
            }
            m_vecSpecChannel.emplace_back(std::move(vecFrom));
        }
        m_vecSpecChannel[uiCodecType][uiFrom][uiTo] = pChannel;
        m_uiCodecSize.store(m_vecSpecChannel.size(), std::memory_order_release);
    }
    return(pSpecChannel);
}

void LaborShared::AddWorkerThreadId(uint64 ullThreadId)
{
    std::lock_guard<std::mutex> guard(s_mutex);
    if (std::find(m_vecWorkerThreadId.begin(), m_vecWorkerThreadId.end(), ullThreadId)
            == m_vecWorkerThreadId.end())
    {
        m_vecWorkerThreadId.push_back(ullThreadId);
    }
}

} /* namespace neb */

