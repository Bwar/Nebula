/*******************************************************************************
 * Project:  Nebula
 * @file     CodecCass.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#include "CodecCass.hpp"
#include "type/Notations.hpp"
#include "CassRequest.hpp"
#include "CassResponse.hpp"
#include "channel/SocketChannel.hpp"

namespace neb
{

CodecCass::CodecCass(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
        std::shared_ptr<SocketChannel> pBindChannel)
    : Codec(pLogger, eCodecType, pBindChannel)
{
}

CodecCass::~CodecCass()
{
}

E_CODEC_STATUS CodecCass::Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    GenerateStreamId();
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecCass::Encode(const CassMessage& oCassMsg, CBuffer* pBuff)
{
    CassFrameHead stFrameHead;
    if (CASS_MSG_RESPONSE == oCassMsg.GetMsgType())
    {
        LOG4_WARNING("cass client only");
        return(CODEC_STATUS_ERR);
    }
    else
    {
        stFrameHead.ucVersion = m_ucVersion;
        stFrameHead.ucVersion |= CASS_MSG_REQUEST;
        stFrameHead.uiStream = GenerateStreamId();
        auto& oRequest = static_cast<const CassRequest&>(oCassMsg);
        CBuffer oBuff;
        if (!m_bStartup)
        {
            stFrameHead.ucOpCode = CASS_OP_STARTUP;
            oRequest.EncodeStarup(&oBuff, "3.0.0");
            stFrameHead.uiLength = oBuff.ReadableBytes();
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(oBuff.GetRawReadBuffer(), oBuff.ReadableBytes());
            oBuff.AdvanceReadIndex(oBuff.ReadableBytes());
            m_bStartup = true;
        }
        if (stFrameHead.uiStream == 0)
        {
            LOG4_ERROR("unable to assign stream id.");
            return(CODEC_STATUS_PART_ERR);
        }
        stFrameHead.ucOpCode = oRequest.GetOpcode();
        switch (stFrameHead.ucOpCode)
        {
            case CASS_OP_QUERY:
                oRequest.EncodeQuery(&oBuff);
                break;
            default:
                LOG4_WARNING("TODO opcode %d", (int)stFrameHead.ucOpCode);
        }
        stFrameHead.uiLength = oBuff.ReadableBytes();
        if (m_bIsReady)
        {
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(oBuff.GetRawReadBuffer(), oBuff.ReadableBytes());
            oBuff.AdvanceReadIndex(oBuff.ReadableBytes());
        }
        else
        {
            EncodeFrameHeader(stFrameHead, &m_oReqBuff);
            m_oReqBuff.Write(oBuff.GetRawReadBuffer(), oBuff.ReadableBytes());
            oBuff.AdvanceReadIndex(oBuff.ReadableBytes());
        }
        return(CODEC_STATUS_OK);
    }
}

E_CODEC_STATUS CodecCass::Encode(const CassMessage& oCassMsg, CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(Encode(oCassMsg, pBuff));
}

E_CODEC_STATUS CodecCass::Decode(CBuffer* pBuff, CassMessage& oCassMsg)
{
    LOG4_ERROR("invalid");
    return(CODEC_STATUS_ERR);
}

E_CODEC_STATUS CodecCass::Decode(CBuffer* pBuff, CassMessage& oCassMsg, CBuffer* pReactBuff)
{
    if (pBuff->ReadableBytes() < 9)
    {
        return(CODEC_STATUS_PAUSE);
    }
    uint32 uiReadIndex = pBuff->GetReadIndex();
    CassFrameHead stFrameHead;
    DecodeFrameHeader(pBuff, stFrameHead);
    if (pBuff->ReadableBytes() < stFrameHead.uiLength)
    {
        pBuff->SetReadIndex(uiReadIndex);
        return(CODEC_STATUS_PAUSE);
    }
    LOG4_TRACE("op code %d, stream %u, body len %u",
            (int)stFrameHead.ucOpCode, stFrameHead.uiStream, stFrameHead.uiLength);
    oCassMsg.m_ucOpcode = stFrameHead.ucOpCode;
    oCassMsg.m_unStreamId = stFrameHead.uiStream;
    auto iter = m_setReqStreamId.find(stFrameHead.uiStream);
    if (iter != m_setReqStreamId.end())
    {
        m_setReqStreamId.erase(iter);   // TODO:  需确认stream是否接收完不同page的响应
    }
    if (stFrameHead.ucVersion & CASS_MSG_RESPONSE)  // response
    {
        oCassMsg.m_unMsgType = CASS_MSG_RESPONSE;
        auto& oResponse = static_cast<CassResponse&>(oCassMsg);
        CBuffer oBuff;
        oBuff.Write(pBuff->GetRawReadBuffer(), stFrameHead.uiLength);
        pBuff->SkipBytes(stFrameHead.uiLength);
        switch (stFrameHead.ucOpCode)
        {
            case CASS_OP_RESULT:
                if (!oResponse.DecodeResult(&oBuff))
                {
                    LOG4_ERROR("cass response CASS_OP_RESULT decode failed.");
                    return(CODEC_STATUS_PART_ERR);
                }
                break;
            case CASS_OP_ERROR:
                if (!oResponse.DecodeError(&oBuff))
                {
                    LOG4_ERROR("cass response CASS_OP_ERROR decode failed.");
                    return(CODEC_STATUS_PART_ERR);
                }
                break;
            case CASS_OP_AUTHENTICATE:
                break;
            case CASS_OP_SUPPORTED:
                break;
            case CASS_OP_EVENT:
                break;
            case CASS_OP_READY:
                m_bIsReady = true;
                if (m_oReqBuff.ReadableBytes() > 0)
                {
                    pReactBuff->Write(m_oReqBuff.GetRawReadBuffer(), m_oReqBuff.ReadableBytes());
                }
                LOG4_INFO("%s cass connection ready.", GetBindChannel()->GetRemoteAddr().c_str());
                break;
            case CASS_OP_AUTH_CHALLENGE:
                break;
            case CASS_OP_AUTH_SUCCESS:
                break;
            default:
                LOG4_ERROR("invalid op code %d in response message.", stFrameHead.ucOpCode);
                return(CODEC_STATUS_ERR);
        }
        return(CODEC_STATUS_OK);
    }
    else    // request
    {
        LOG4_WARNING("cass client only");
        oCassMsg.m_unMsgType = CASS_MSG_REQUEST;
        auto& oRequest = static_cast<CassRequest&>(oCassMsg);
        if (CASS_FLAG_TRACING & stFrameHead.ucFlags)
        {
            if (!Notations::DecodeUuid(pBuff, oRequest.m_strTracingId))
            {
                LOG4_ERROR("failed to decode uuid");
                return(CODEC_STATUS_PART_ERR);
            }
        }
        if (CASS_FLAG_WARNING & stFrameHead.ucFlags)
        {
            if (!Notations::DecodeStringList(pBuff, oRequest.m_vecWarnings))
            {
                LOG4_ERROR("failed to decode warnings");
                return(CODEC_STATUS_PART_ERR);
            }
        }
        if (CASS_FLAG_CUSTOM_PAYLOAD & stFrameHead.ucFlags)
        {
            if (!Notations::DecodeBytesMap(pBuff, oRequest.m_mapCustomPayload))
            {
                LOG4_ERROR("failed to decode warnings");
                return(CODEC_STATUS_PART_ERR);
            }
        }
        return(CODEC_STATUS_ERR);   // TODO
    }
}

uint16 CodecCass::GenerateStreamId()
{
    int iLoopNum = 0;
    ++m_unLastReqStreamId;
    m_unLastReqStreamId = (m_unLastReqStreamId > 0) ? m_unLastReqStreamId : 1;
    auto iter = m_setReqStreamId.find(m_unLastReqStreamId);
    while (iter != m_setReqStreamId.end())
    {
        ++m_unLastReqStreamId;
        if (m_unLastReqStreamId > 32768)
        {
            if (iLoopNum > 0)
            {
                return(0);
            }
            ++iLoopNum;
            m_unLastReqStreamId = 1;
        }
        iter = m_setReqStreamId.find(m_unLastReqStreamId);
    }
    return(m_unLastReqStreamId);
}

void CodecCass::DecodeFrameHeader(CBuffer* pBuff, CassFrameHead& stFrameHead)
{
    pBuff->Read(&stFrameHead.ucVersion, 1);
    pBuff->Read(&stFrameHead.ucFlags, 1);
    pBuff->Read(&stFrameHead.uiStream, 2);
    pBuff->Read(&stFrameHead.ucOpCode, 1);
    pBuff->Read(&stFrameHead.uiLength, 4);
    stFrameHead.uiStream = CodecUtil::N2H(stFrameHead.uiStream);
    stFrameHead.uiLength = CodecUtil::N2H(stFrameHead.uiLength);
}

void CodecCass::EncodeFrameHeader(const CassFrameHead& stFrameHead, CBuffer* pBuff)
{
    uint16 uiStream = CodecUtil::H2N(stFrameHead.uiStream);
    uint32 uiLength = CodecUtil::H2N(stFrameHead.uiLength);
    pBuff->Write(&stFrameHead.ucVersion, 1);
    pBuff->Write(&stFrameHead.ucFlags, 1);
    pBuff->Write(&uiStream, 2);
    pBuff->Write(&stFrameHead.ucOpCode, 1);
    pBuff->Write(&uiLength, 4);
}

} /* namespace neb */

