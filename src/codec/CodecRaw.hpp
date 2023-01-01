/*******************************************************************************
 * Project:  Nebula
 * @file     CodecRaw.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECRAW_HPP_
#define SRC_CODEC_CODECRAW_HPP_

#include "Codec.hpp"
#include "type/Notations.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"

namespace neb
{

class CodecRaw: public Codec
{
public:
    CodecRaw(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecRaw();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_RAW);
    }

    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const char* pRaw, uint32 uiRawSize);

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const char* pRaw, uint32 uiRawSize);
    
    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const char* pRaw, uint32 uiRawSize, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const char* pRaw, uint32 uiRawSize, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CBuffer& oBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CBuffer& oBuff, CBuffer* pReactBuff);
};

// request
template<typename ...Targs>
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
template<typename ...Targs>
int CodecRaw::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const char* pRaw, uint32 uiRawSize)
{
    uint32 uiFrom;
    uint32 uiTo;
    std::static_pointer_cast<SpecChannel<Bytes>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiTo, uiFrom, uiFlags, uiStepSeq, pRaw, uiRawSize));
}

} /* namespace neb */

#endif /* SRC_CODEC_CODECRAW_HPP_ */
