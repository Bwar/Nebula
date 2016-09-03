/*******************************************************************************
 * Project:  Nebula
 * @file     CodecWebSocketExtent.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年9月2日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECWSEXTENTPB_HPP_
#define SRC_CODEC_CODECWSEXTENTPB_HPP_

#include "Codec.hpp"

namespace neb
{

const uint8 WEBSOCKET_FIN                   = 0x80;
const uint8 WEBSOCKET_RSV1                  = 0x40;
const uint8 WEBSOCKET_RSV2                  = 0x20;
const uint8 WEBSOCKET_RSV3                  = 0x10;
const uint8 WEBSOCKET_OPCODE                = 0x0F;
const uint8 WEBSOCKET_FRAME_CONTINUE        = 0;
const uint8 WEBSOCKET_FRAME_TEXT            = 1;
const uint8 WEBSOCKET_FRAME_BINARY          = 2;
// %x3-7 保留用于未来的非控制帧
const uint8 WEBSOCKET_FRAME_CLOSE           = 8;
const uint8 WEBSOCKET_FRAME_PING            = 9;
const uint8 WEBSOCKET_FRAME_PONG            = 10;
// %xB-F 保留用于未来的控制帧
const uint8 WEBSOCKET_MASK                  = 0x80;
const uint8 WEBSOCKET_PAYLOAD_LEN           = 0x7F;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT16    = 126;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT64    = 127;

class CodecWsExtentPb: public Codec
{
public:
    CodecWsExtentPb(E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~CodecWsExtentPb();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, loss::CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(loss::CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);

private:
    uint32 uiBeatCmd;
    uint32 uiBeatSeq;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECWSEXTENTPB_HPP_ */
