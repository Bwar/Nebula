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

Http2Stream::Http2Stream(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, uint32 uiStreamId)
    : Codec(pLogger, eCodecType),
      m_eStreamState(H2_STREAM_IDLE), m_uiStreamId(uiStreamId), m_bEndHeaders(false), m_pFrame(nullptr)
{
#if __cplusplus >= 201401L
    m_pFrame = std::make_unique<Http2Frame>(pLogger, eCodecType, this);
#else
    m_pFrame = std::unique_ptr<Http2Frame>(new Http2Frame(pLogger, eCodecType, this));
#endif
}

Http2Stream::~Http2Stream()
{
    LOG4_TRACE("codec type %d", GetCodecType());
}

E_CODEC_STATUS Http2Stream::Encode(CodecHttp2* pCodecH2,
        const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    tagPriority stPriority;
    std::string strPadding;
    if (HTTP_REQUEST == oHttpMsg.type())
    {
        m_bChunkNotice = oHttpMsg.chunk_notice();
        stPriority.uiDependency = 0;
        stPriority.ucWeight = 15;
        std::string strScheme = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(':'));
        if (strScheme.size() > 0)
        {
            auto pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
            pHeader->set_name(":scheme");
            pHeader->set_value(strScheme);
        }
        else
        {
            auto pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
            pHeader->set_name(":scheme");
            pHeader->set_value("http");
        }
        struct http_parser_url stUrl;
        if(0 == http_parser_parse_url(oHttpMsg.url().c_str(), oHttpMsg.url().length(), 0, &stUrl))
        {
            std::string strAuthority;
            if(stUrl.field_set & (1 << UF_USERINFO) )
            {
                strAuthority += oHttpMsg.url().substr(stUrl.field_data[UF_USERINFO].off, stUrl.field_data[UF_USERINFO].len);
                strAuthority += "@";
            }
            if(stUrl.field_set & (1 << UF_HOST) )
            {
                strAuthority += oHttpMsg.url().substr(stUrl.field_data[UF_HOST].off, stUrl.field_data[UF_HOST].len);
            }
            if(stUrl.field_set & (1 << UF_PORT))
            {
                strAuthority += ":";
                strAuthority += oHttpMsg.url().substr(stUrl.field_data[UF_PORT].off, stUrl.field_data[UF_PORT].len);
            }
            auto pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
            pHeader->set_name(":authority");
            pHeader->set_value(strAuthority);
            std::string strPath = "/";
            if(stUrl.field_set & (1 << UF_PATH))
            {
                //strPath = oHttpMsg.url().substr(stUrl.field_data[UF_PATH].off, stUrl.field_data[UF_PATH].len);
                strPath = oHttpMsg.url().substr(stUrl.field_data[UF_PATH].off);     // including: ?param1=aaa&param2=bbb
            }
            pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
            pHeader->set_name(":path");
            pHeader->set_value(strPath);
        }
        auto pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
        pHeader->set_name(":method");
        pHeader->set_value(http_method_str((http_method)oHttpMsg.method()));
        (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"user-agent", "grpc-c++-nebula"});
        (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"grpc-accept-encoding", "gzip"});
    }
    else
    {
        auto pHeader = (const_cast<HttpMsg&>(oHttpMsg)).add_pseudo_header();
        pHeader->set_name(":status");
        pHeader->set_value(std::to_string(oHttpMsg.status_code()));
    }
    return(m_pFrame->Encode(pCodecH2, oHttpMsg, stPriority, strPadding, pBuff));
}

E_CODEC_STATUS Http2Stream::Decode(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    LOG4_TRACE("m_eStreamState = %d, stFrameHead.ucType = %u", (int)m_eStreamState, (uint32)stFrameHead.ucType);
    m_oHttpMsg.set_stream_id(m_uiStreamId);
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    eStatus = m_pFrame->Decode(pCodecH2, stFrameHead, pBuff, m_oHttpMsg, pReactBuff);
    switch (m_eStreamState)
    {
        case H2_STREAM_IDLE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    m_eStreamState = H2_STREAM_OPEN;
                    break;
                case H2_FRAME_PUSH_PROMISE:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_RESERVED_REMOTE);
                    m_eStreamState = H2_STREAM_RESERVED_REMOTE;
                    break;
                default:
                    pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
                    m_pFrame->EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR,
                            "The endpoint detected an unspecific protocol error."
                            " This error is for use when a more specific error "
                            "code is not available.", pReactBuff);
                    LOG4_ERROR("m_eStreamState = %d, stFrameHead.ucType = %u", (int)m_eStreamState, (uint32)stFrameHead.ucType);
                    return(CODEC_STATUS_ERR);
            }
            if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
            {
                m_bEndHeaders = true;
            }
            break;
        case H2_STREAM_RESERVED_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_HALF_CLOSE_LOCAL);
                    m_eStreamState = H2_STREAM_HALF_CLOSE_LOCAL;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_RST_STREAM:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
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
                LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_HALF_CLOSE_REMOTE);
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                case H2_FRAME_WINDOW_UPDATE:
                case H2_FRAME_PRIORITY:
                    break;
                default:
                    LOG4_ERROR("m_eStreamState = %d, stFrameHead.ucType = %u", (int)m_eStreamState, (uint32)stFrameHead.ucType);
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
                    LOG4_ERROR("m_eStreamState = %d, stFrameHead.ucType = %u", (int)m_eStreamState, (uint32)stFrameHead.ucType);
                    return(CODEC_STATUS_PART_ERR);
            }
            break;
        default:
            break;
    }
    LOG4_TRACE("m_eStreamState = %d, stFrameHead.ucType = %u, m_bEndHeaders = %d", m_eStreamState, (uint32)stFrameHead.ucType, m_bEndHeaders);
    if (m_bEndHeaders)
    {
        if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PART_OK == eStatus)
        {
            if (pCodecH2->IsClient())
            {
                if (m_oHttpMsg.stream_id() & 0x01)
                {
                    m_oHttpMsg.set_type(HTTP_RESPONSE);
                }
                else
                {
                    m_oHttpMsg.set_type(HTTP_REQUEST);
                }
            }
            else
            {
                if (m_oHttpMsg.stream_id() & 0x01)
                {
                    m_oHttpMsg.set_type(HTTP_REQUEST);
                }
                else
                {
                    m_oHttpMsg.set_type(HTTP_RESPONSE);
                }
            }
            if (m_bChunkNotice)
            {
                oHttpMsg = std::move(m_oHttpMsg);
            }
            else
            {
                if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
                {
                    oHttpMsg = std::move(m_oHttpMsg);
                    eStatus = CODEC_STATUS_OK;
                }
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
            LOG4_ERROR("m_eStreamState = %d, stFrameHead.ucType = %u, m_bEndHeaders = %d", m_eStreamState, (uint32)stFrameHead.ucType, m_bEndHeaders);
            return(CODEC_STATUS_ERR);
        }
    }
    return(eStatus);
}

void Http2Stream::EncodeSetState(const tagH2FrameHead& stFrameHead)
{
    LOG4_TRACE("stream %u m_eStreamState = %d, stFrameHead.ucType = %u", m_uiStreamId, m_eStreamState, (uint32)stFrameHead.ucType);
    switch (m_eStreamState)
    {
        case H2_STREAM_IDLE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_OPEN);
                    m_eStreamState = H2_STREAM_OPEN;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_PUSH_PROMISE:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_RESERVED_LOCAL);
                    m_eStreamState = H2_STREAM_RESERVED_LOCAL;
                    break;
                default:
                    LOG4_ERROR("protocol error %d", H2_ERR_PROTOCOL_ERROR);
            }
            break;
        case H2_STREAM_RESERVED_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_HEADERS:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_HALF_CLOSE_REMOTE);
                    m_eStreamState = H2_STREAM_HALF_CLOSE_REMOTE;
                    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
                    {
                        m_bEndHeaders = true;
                    }
                    break;
                case H2_FRAME_RST_STREAM:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            break;
        case H2_STREAM_RESERVED_REMOTE:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_HALF_CLOSE_REMOTE);
                m_eStreamState = H2_STREAM_HALF_CLOSE_LOCAL;
            }
            break;
        case H2_STREAM_HALF_CLOSE_LOCAL:
            switch (stFrameHead.ucType)
            {
                case H2_FRAME_RST_STREAM:
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
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
                    LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                    m_eStreamState = H2_STREAM_CLOSE;
                    break;
                default:
                    break;
            }
            if (H2_FRAME_FLAG_END_STREAM & stFrameHead.ucFlag)
            {
                LOG4_TRACE("stream %u state from %d to %d", m_uiStreamId, m_eStreamState, H2_STREAM_CLOSE);
                m_eStreamState = H2_STREAM_CLOSE;
            }
            break;
        case H2_STREAM_CLOSE:
            break;
        default:
            break;
    }
}

void Http2Stream::WindowInit(uint32 uiWindowSize)
{
    m_iSendWindowSize = (int32)uiWindowSize;
}

void Http2Stream::WindowUpdate(int32 iIncrement)
{
    m_iSendWindowSize += iIncrement;
}

void Http2Stream::UpdateRecvWindow(CodecHttp2* pCodecH2, uint32 uiStreamId, uint32 uiRecvLength, CBuffer* pBuff)
{
    m_uiRecvWindowSize -= uiRecvLength;
    if (m_eStreamState == H2_STREAM_OPEN
            || m_eStreamState == H2_STREAM_IDLE
            || m_eStreamState == H2_STREAM_RESERVED_LOCAL
            || m_eStreamState == H2_STREAM_RESERVED_REMOTE)
    {
        if (m_uiRecvWindowSize < DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE / 4)
        {
            m_pFrame->EncodeWindowUpdate(pCodecH2, uiStreamId,
                    SETTINGS_MAX_INITIAL_WINDOW_SIZE - uiRecvLength, pBuff);
            m_uiRecvWindowSize = SETTINGS_MAX_INITIAL_WINDOW_SIZE;
        }
    }
}

E_CODEC_STATUS Http2Stream::SendWaittingFrameData(CodecHttp2* pCodecH2, CBuffer* pBuff)
{
    return(m_pFrame->SendWaittingFrameData(pCodecH2, pBuff));
}

} /* namespace neb */

