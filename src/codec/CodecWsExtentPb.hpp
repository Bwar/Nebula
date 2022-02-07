/*******************************************************************************
 * Project:  Nebula
 * @file     CodecWebSocketExtent.hpp
 * @brief    WebSocket编解码器
 * @author   Bwar
 * @date:    2016年9月2日
 * @note     WebSocket编解码器，payload部分数据带自定义扩展数据，Extension data
 * 为自定义二进制包头tagMsgHead，Application data为protobuf协议MsgBody体。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECWSEXTENTPB_HPP_
#define SRC_CODEC_CODECWSEXTENTPB_HPP_

#include "Codec.hpp"

namespace neb
{

class CodecWsExtentPb: public Codec
{
public:
    CodecWsExtentPb(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecWsExtentPb();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_WS_EXTEND_PB);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff);
    E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody, CBuffer* pReactBuff);

private:
    uint32 uiBeatCmd;
    uint32 uiBeatSeq;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECWSEXTENTPB_HPP_ */
