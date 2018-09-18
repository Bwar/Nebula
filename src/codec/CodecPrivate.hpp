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
    CodecPrivate(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~CodecPrivate();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECPRIVATE_HPP_ */
