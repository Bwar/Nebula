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

    E_CODEC_STATUS Encode(CBuffer* pBuff);
    E_CODEC_STATUS Encode(const char* pRaw, uint32 uiRawSize, CBuffer* pBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CBuffer& oBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, CBuffer& oBuff, CBuffer* pReactBuff);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECRAW_HPP_ */
