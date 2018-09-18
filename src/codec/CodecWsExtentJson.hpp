/*******************************************************************************
 * Project:  Nebula
 * @file     CodecWsExtentJson.hpp
 * @brief    WebSocket编解码器
 * @author   Bwar
 * @date:    2016年9月3日
 * @note     WebSocket编解码器，payload部分数据带自定义扩展数据，Extension data
 * 为自定义二进制包头tagMsgHead，Application data为json体。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECWSEXTENTJSON_HPP_
#define SRC_CODEC_CODECWSEXTENTJSON_HPP_

#include "CodecUtil.hpp"
#include "Codec.hpp"

namespace neb
{

class CodecWsExtentJson: public Codec
{
public:
    CodecWsExtentJson(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~CodecWsExtentJson();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);

private:
    uint32 uiBeatCmd;
    uint32 uiBeatSeq;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECWSEXTENTJSON_HPP_ */
