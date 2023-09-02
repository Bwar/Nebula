/*******************************************************************************
 * Project:  Nebula
 * @file     CodecPrivate.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECPRIVATE_HPP_
#define SRC_CODEC_CODECPRIVATE_HPP_

#include "Codec.hpp"

namespace neb
{

class CodecPrivate: public Codec
{
public:
    CodecPrivate(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecPrivate();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_PRIVATE);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody, CBuffer* pReactBuff);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECPRIVATE_HPP_ */
