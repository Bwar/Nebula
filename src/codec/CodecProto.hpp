/*******************************************************************************
 * Project:  Nebula
 * @file     CodecProto.hpp
 * @brief    protobuf编解码器
 * @author   Bwar
 * @date:    2016年8月11日
 * @note     对应proto里msg.proto的MsgHead和MsgBody
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECPROTO_HPP_
#define SRC_CODEC_CODECPROTO_HPP_

#include "Codec.hpp"

namespace neb
{

class CodecProto: public Codec
{
public:
    CodecProto(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~CodecProto();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECPROTO_HPP_ */
