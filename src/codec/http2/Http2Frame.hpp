/*******************************************************************************
 * Project:  Nebula
 * @file     Http2Frame.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-02
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_HTTP2FRAME_HPP_
#define SRC_CODEC_HTTP2_HTTP2FRAME_HPP_

#include <string>
#include "Definition.hpp"
#include "codec/Codec.hpp"
#include "pb/http.pb.h"
#include "H2Comm.hpp"

namespace neb
{

enum E_H2_FRAME_TYPE
{
    H2_FRAME_DATA                           = 0x0,  ///< 帧包含消息的所有或部分有效负载。
    H2_FRAME_HEADERS                        = 0x1,  ///< 帧仅包含 HTTP 标头信息。
    H2_FRAME_PRIORITY                       = 0x2,  ///< 指定分配给流的优先级。
    H2_FRAME_RST_STREAM                     = 0x3,  ///< 错误通知：一个推送承诺遭到拒绝。终止流。
    H2_FRAME_SETTINGS                       = 0x4,  ///< 指定连接配置。
    H2_FRAME_PUSH_PROMISE                   = 0x5,  ///< 通知一个将资源推送到客户端的意图。
    H2_FRAME_PING                           = 0x6,  ///< 检测信号和往返时间。
    H2_FRAME_GOAWAY                         = 0x7,  ///< 停止为当前连接生成流的停止通知。
    H2_FRAME_WINDOW_UPDATE                  = 0x8,  ///< 用于管理流的流控制。
    H2_FRAME_CONTINUATION                   = 0x9,  ///< 用于延续某个标头碎片序列。
};

enum E_H2_FRAME_FLAG
{
    H2_FRAME_FLAG_END_STREAM                = 0x1,  ///< END_STREAM (0x1)
    H2_FRAME_FLAG_ACK                       = 0x1,  ///< SETTING ACK (0x1)
    H2_FRAME_FLAG_END_HEADERS               = 0x4,  ///< END_HEADERS (0x4)
    H2_FRAME_FLAG_PADDED                    = 0x8,  ///< PADDED (0x8)
    H2_FRAME_FLAG_PRIORITY                  = 0x20, ///< PRIORITY（0x20）
};

class CodecHttp2;
class Http2Stream;

class Http2Frame: public Codec
{
public:
    Http2Frame(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, Http2Stream* pStream = nullptr);
    virtual ~Http2Frame();

    static void WriteMediumInt(uint32 uiValue, CBuffer* pBuff);
    static void ReadMediumInt(CBuffer* pBuff, uint32& uiValue);
    static void DecodeFrameHeader(CBuffer* pBuff, tagH2FrameHead& stFrameHead);
    static void EncodeFrameHeader(const tagH2FrameHead& stFrameHead, CBuffer* pBuff);
    static E_CODEC_STATUS EncodeSetting(const std::vector<tagSetting>& vecSetting, std::string& strSettingFrame);

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
    {
        return(CODEC_STATUS_INVALID);
    }
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
    {
        return(CODEC_STATUS_INVALID);
    }

    virtual E_CODEC_STATUS Encode(CodecHttp2* pCodecH2,
            const HttpMsg& oHttpMsg,
            const tagPriority& stPriority,
            const std::string& strPadding, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);

protected:
    E_CODEC_STATUS DecodeData(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeHeaders(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodePriority(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeRstStream(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeSetting(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            CBuffer* pReactBuff);
    E_CODEC_STATUS DecodePushPromise(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodePing(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeGoaway(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeWindowUpdate(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);
    E_CODEC_STATUS DecodeContinuation(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);


    E_CODEC_STATUS EncodeData(CodecHttp2* pCodecH2,
            uint32 uiStreamId, const HttpMsg& oHttpMsg, bool bEndStream,
            const std::string& strPadding, CBuffer* pBuff);
    E_CODEC_STATUS EncodeHeaders(CodecHttp2* pCodecH2,
            uint32 uiStreamId, const HttpMsg& oHttpMsg,
            const tagPriority& stPriority, const std::string& strPadding,
            bool bEndStream, CBuffer* pBuff);
    E_CODEC_STATUS EncodePriority(CodecHttp2* pCodecH2,
            uint32 uiStreamId, const tagPriority& stPriority, CBuffer* pBuff);
    E_CODEC_STATUS EncodeRstStream(CodecHttp2* pCodecH2,
            uint32 uiStreamId, int32 iErrCode, CBuffer* pBuff);
    E_CODEC_STATUS EncodeSetting(CodecHttp2* pCodecH2,
            const std::vector<tagSetting>& vecSetting, CBuffer* pBuff);
    E_CODEC_STATUS EncodeSetting(CodecHttp2* pCodecH2, CBuffer* pBuff); // ACK
    E_CODEC_STATUS EncodePushPromise(CodecHttp2* pCodecH2,
            uint32 uiStreamId, uint32 uiPromiseStreamId,
            const std::string& strPadding, const HttpMsg& oHttpMsg,
            bool bEndStream, CBuffer* pBuff);
    E_CODEC_STATUS EncodePing(CodecHttp2* pCodecH2,
            bool bAck, int32 iPayload1, int32 iPayload2, CBuffer* pBuff);
    E_CODEC_STATUS EncodeGoaway(CodecHttp2* pCodecH2,
            int32 iErrCode, const std::string& strDebugData, CBuffer* pBuff);
    E_CODEC_STATUS EncodeWindowUpdate(CodecHttp2* pCodecH2,
            uint32 uiStreamId, uint32 uiIncrement, CBuffer* pBuff);
    E_CODEC_STATUS EncodeContinuation(CodecHttp2* pCodecH2,
            uint32 uiStreamId, bool bEndStream, CBuffer* pHpackBuff, CBuffer* pBuff);

protected:
    void EncodePriority(const tagPriority& stPriority, CBuffer* pBuff);
    void EncodeSetStreamState(const tagH2FrameHead& stFrameHead);
    E_CODEC_STATUS EncodeData(CodecHttp2* pCodecH2, uint32 uiStreamId,
            const char* pData, uint32 uiDataLen, bool bEndStream,
            const std::string& strPadding, uint32& uiEncodedDataLen, CBuffer* pBuff);

private:
    friend class CodecHttp2;
    friend class Http2Stream;

    Http2Stream* m_pStream;
};

} /* namespace neb */

#endif /* SRC_CODEC_HTTP2_HTTP2FRAME_HPP_ */

