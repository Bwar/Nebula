/*******************************************************************************
 * Project:  Nebula
 * @file     Http2Stream.cpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-03
 * @note     
 * Modify history:
 ******************************************************************************/
#include "Http2Stream.hpp"
#include "Http2Frame.hpp"
#include "CodecHttp2.hpp"

namespace neb
{

Http2Stream::Http2Stream(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType),
      m_eStreamState(H2_STREAM_IDLE), m_uiStreamId(0)
{
#if __cplusplus >= 201401L
    m_pFrame = std::make_unique<Http2Frame>(pLogger, eCodecType);
#else
    m_pFrame = std::unique_ptr<Http2Frame>(new Http2Frame(pLogger, eCodecType));
#endif
}

Http2Stream::~Http2Stream()
{
}

E_CODEC_STATUS Http2Stream::Encode(CodecHttp2* pCodecH2,
        const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    m_pFrame->Encode(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff);
    switch (m_eStreamState)
    {
        case H2_STREAM_IDLE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    m_eStreamState = H2_STREAM_OPEN;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
                    m_pFrame->EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR,
                            "The endpoint detected an unspecific protocol error."
                            " This error is for use when a more specific error "
                            "code is not available.", pReactBuff);
                    return(CODEC_STATUS_ERR);
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_HALF_CLOSE_REMOTE;
            }
            break;
        case H2_STREAM_RESERVED_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            break;
        case H2_STREAM_RESERVED_REMOTE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    m_eStreamState = H2_STREAM_HALF_CLOSE_LOCAL;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            break;
        case H2_STREAM_OPEN:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_HALF_CLOSE_REMOTE;
            }
            break;
        case H2_STREAM_HALF_CLOSE_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_CLOSE;
            }
            break;
        case H2_STREAM_HALF_CLOSE_REMOTE:
            /**
             * If an endpoint receives additional frames, other than WINDOW_UPDATE,
             * PRIORITY, or RST_STREAM, for a stream that is in this state, it
             * MUST respond with a stream error (Section 5.4.2) of type.
             */
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                case H2_FRAME_WINDOW_UPDATE:
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    m_pFrame->EncodeRstStream(pCodecH2,
                            stFrameHead.uiStreamIdentifier, H2_ERR_STREAM_CLOSED, pReactBuff);
                    return(CODEC_STATUS_PART_ERR);
            }
            break;
        case H2_STREAM_CLOSE:
            break;
        default:
            break;
    }
    return();
}

E_CODEC_STATUS Http2Stream::Decode(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    switch (m_eStreamState)
    {
        case H2_STREAM_IDLE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    m_eStreamState = H2_STREAM_OPEN;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
                    m_pFrame->EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR,
                            "The endpoint detected an unspecific protocol error."
                            " This error is for use when a more specific error "
                            "code is not available.", pReactBuff);
                    return(CODEC_STATUS_ERR);
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_HALF_CLOSE_REMOTE;
            }
            break;
        case H2_STREAM_RESERVED_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            break;
        case H2_STREAM_RESERVED_REMOTE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    m_eStreamState = H2_STREAM_HALF_CLOSE_LOCAL;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            break;
        case H2_STREAM_OPEN:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                case H2_FRAME_CONTINUATION:
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_HALF_CLOSE_REMOTE;
            }
            break;
        case H2_STREAM_HALF_CLOSE_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                case H2_FRAME_CONTINUATION:
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                m_eStreamState = H2_STREAM_CLOSE;
            }
            break;
        case H2_STREAM_HALF_CLOSE_REMOTE:
            /**
             * If an endpoint receives additional frames, other than WINDOW_UPDATE,
             * PRIORITY, or RST_STREAM, for a stream that is in this state, it
             * MUST respond with a stream error (Section 5.4.2) of type.
             */
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                case H2_FRAME_WINDOW_UPDATE:
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    m_pFrame->EncodeRstStream(pCodecH2,
                            stFrameHead.uiStreamIdentifier, H2_ERR_STREAM_CLOSED, pReactBuff);
                    return(CODEC_STATUS_PART_ERR);
            }
            break;
        case H2_STREAM_CLOSE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    m_pFrame->EncodeRstStream(pCodecH2,
                            stFrameHead.uiStreamIdentifier, H2_ERR_STREAM_CLOSED, pReactBuff);
                    return(CODEC_STATUS_PART_ERR);
            }
            break;
        default:
            break;
    }
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    if (m_bEndHeaders)
    {
        eStatus = m_pFrame->Decode(pCodecH2, stFrameHead, pBuff, m_oHttpMsg, pReactBuff);
        m_oHttpMsg.set_stream_id(m_uiStreamId);
        if (pCodecH2->IsChunkNotice())
        {
            oHttpMsg = std::move(m_oHttpMsg);
        }
        else
        {
            if (m_eStreamState == H2_STREAM_CLOSE)
            {
                oHttpMsg = std::move(m_oHttpMsg);
            }
        }
    }
    else
    {
        if (stFrameHead.ucType != H2_FRAME_CONTINUATION)
        {
            // If the END_HEADERS bit is not set, this frame MUST be followed by another CONTINUATION frame.
            // A receiver MUST treat the receipt of any other type of frame or a frame on a different
            // stream as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
            SetErrno(H2_ERR_PROTOCOL_ERROR);
            m_pFrame->EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                    "detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        eStatus = m_pFrame->Decode(pCodecH2, stFrameHead, pBuff, m_oHttpMsg, pReactBuff);
    }
    return(eStatus);
}

} /* namespace neb */

