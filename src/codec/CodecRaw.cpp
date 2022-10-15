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
