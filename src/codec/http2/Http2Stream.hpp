/*******************************************************************************
 * Project:  Nebula
 * @file     Http2Stream.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_HTTP2STREAM_HPP_
#define SRC_CODEC_HTTP2_HTTP2STREAM_HPP_

#include <unordered_map>
#include "Definition.hpp"
#include "codec/Codec.hpp"
#include "pb/http.pb.h"
#include "H2Comm.hpp"

namespace neb
{

const uint32 HTTP2_CLIENT_STREAM = 0x00000001;

enum E_H2_STREAM_STATES
{
    H2_STREAM_IDLE                          = 0,
    H2_STREAM_RESERVED_LOCAL                = 1,
    H2_STREAM_RESERVED_REMOTE               = 2,
    H2_STREAM_OPEN                          = 3,
    H2_STREAM_HALF_CLOSE_LOCAL              = 4,
    H2_STREAM_HALF_CLOSE_REMOTE             = 5,
    H2_STREAM_CLOSE                         = 6,
};

class Http2Frame;
class CodecHttp2;

class Http2Stream: public Codec
{
public:
    Http2Stream(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~Http2Stream();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
    {
        return(CODEC_STATUS_INVALID);
    }
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
    {
        return(CODEC_STATUS_INVALID);
    }

    virtual E_CODEC_STATUS Encode(CodecHttp2* pCodecH2,
            const HttpMsg& oHttpMsg, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CodecHttp2* pCodecH2,
            const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
            HttpMsg& oHttpMsg, CBuffer* pReactBuff);

    uint32 GetStreamId() const
    {
        return(m_uiStreamId);
    }

    bool IsEndHeaders() const
    {
        return(m_bEndHeaders);
    }

    void SetState(E_H2_STREAM_STATES eStreamState)
    {
        m_eStreamState = eStreamState;
    }

private:
    E_H2_STREAM_STATES m_eStreamState;
    uint32 m_uiStreamId;
    bool m_bEndHeaders = false;
    std::unique_ptr<Http2Frame> m_pFrame;
    HttpMsg m_oHttpMsg;
};

} /* namespace neb */

#endif /* SRC_CODEC_HTTP2_HTTP2STREAM_HPP_ */

