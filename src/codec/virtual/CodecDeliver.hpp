/*******************************************************************************
 * Project:  Nebula
 * @file     CodecDeliver.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-25
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_
#define SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_

#include <memory>
#include "codec/Codec.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"
#include "type/Package.hpp"

namespace neb
{

class CodecDeliver: public Codec
{
public:
    CodecDeliver(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecDeliver();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_DELIVER);
    }

    // request
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage);

    // response
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage);

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(Package&& oPackage, CBuffer* pBuff);
    E_CODEC_STATUS Encode(Package&& oPackage, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, Package&& oPackage);
    E_CODEC_STATUS Decode(CBuffer* pBuff, Package&& oPackage, CBuffer* pReactBuff);
};

} /* namespace neb */

#endif /* SRC_CODEC_VIRTUAL_CODECDELIVER_HPP_ */

