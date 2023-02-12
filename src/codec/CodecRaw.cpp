/*******************************************************************************
 * Project:  Nebula
 * @file     CodecRaw.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#include <netinet/in.h>
#include "logger/NetLogger.hpp"
#include "CodecRaw.hpp"
#include "CodecDefine.hpp"

namespace neb
{

CodecRaw::CodecRaw(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
        std::shared_ptr<SocketChannel> pBindChannel)
    : Codec(pLogger, eCodecType, pBindChannel)
{
}

CodecRaw::~CodecRaw()
{
}

// request
int CodecRaw::Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const char* pRaw, uint32 uiRawSize)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<Bytes>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(Type(), uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<Bytes>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, Type());
        Bytes oBytes;
        oBytes.Assign(pRaw, uiRawSize);
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(oBytes));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(Type(), uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<Bytes>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        Bytes oBytes;
        oBytes.Assign(pRaw, uiRawSize);
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(oBytes));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

// response
int CodecRaw::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const char* pRaw, uint32 uiRawSize)
{
    uint32 uiFrom;
    uint32 uiTo;
    std::static_pointer_cast<SpecChannel<Bytes>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiTo, uiFrom, uiFlags, uiStepSeq, pRaw, uiRawSize));
}

E_CODEC_STATUS CodecRaw::Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecRaw::Encode(const char* pRaw, uint32 uiRawSize, CBuffer* pBuff)
{
    if (pRaw == nullptr)
    {
        return(CODEC_STATUS_ERR);
    }
    pBuff->Write(pRaw, uiRawSize);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecRaw::Encode(const char* pRaw, uint32 uiRawSize, CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(Encode(pRaw, uiRawSize, pBuff));
}

E_CODEC_STATUS CodecRaw::Decode(CBuffer* pBuff, CBuffer& oBuff)
{
    oBuff.Write(pBuff->GetRawReadBuffer(), pBuff->ReadableBytes());
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecRaw::Decode(CBuffer* pBuff, CBuffer& oBuff, CBuffer* pReactBuff)
{
    LOG4_ERROR("invalid");
    return(CODEC_STATUS_ERR);
}

} /* namespace neb */
