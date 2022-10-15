/*******************************************************************************
 * Project:  Nebula
 * @file     CodecHttp2.cpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-01
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CodecHttp2.hpp"
#include <algorithm>
#include "Http2Stream.hpp"
#include "Http2Frame.hpp"
#include "Http2Header.hpp"
#include "codec/CodecUtil.hpp"
#include "util/StringConverter.hpp"
#include "channel/SocketChannel.hpp"

namespace neb
{

CodecHttp2::CodecHttp2(std::shared_ptr<NetLogger> pLogger,
        E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel)
    : Codec(pLogger, eCodecType, pBindChannel)
{
    try
    {
        m_pFrame = new Http2Frame(pLogger, eCodecType, pBindChannel);
        m_pStreamWeightRoot = new TreeNode<tagStreamWeight>();
        m_pStreamWeightRoot->pData = new tagStreamWeight();
        m_pStreamWeightRoot->pData->uiStreamId = 0;

        Http2Header oHeader("", "");
        m_vecEncodingDynamicTable.assign(8, oHeader);
        m_iNextEncodingDynamicTableHeaderIndex = m_vecEncodingDynamicTable.size() - 1;
        m_vecDecodingDynamicTable.assign(8, oHeader);
        m_iNextDecodingDynamicTableHeaderIndex = m_vecDecodingDynamicTable.size() - 1;
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
    }
}

CodecHttp2::~CodecHttp2()
{
    ReleaseStreamWeight(m_pStreamWeightRoot);
    m_pStreamWeightRoot = nullptr;
    for (auto iter = m_mapStream.begin(); iter != m_mapStream.end(); ++iter)
    {
        delete iter->second;
        iter->second = nullptr;
    }
    m_mapStream.clear();
    if (m_pFrame != nullptr)
    {
        delete m_pFrame;
        m_pFrame = nullptr;
    }
    LOG4_TRACE("codec type %d", GetCodecType());
}

void CodecHttp2::ConnectionSetting(CBuffer* pBuff)
{
    std::vector<tagSetting> vecSetting;
    tagSetting stSetting;
    if (GetBindChannel()->IsClient())
    {
        pBuff->Write("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
        m_bWantMagic = false;
        stSetting.unIdentifier = H2_SETTINGS_INITIAL_WINDOW_SIZE;
        stSetting.uiValue = 1048576;
        m_uiRecvWindowSize = 1048576;
        m_uiSendWindowSize = 1048576;
        vecSetting.push_back(stSetting);
        stSetting.unIdentifier = H2_SETTINGS_MAX_FRAME_SIZE;
        stSetting.uiValue = 4194304;
        m_uiSettingsMaxDecodeFrameSize = 4194304;
        vecSetting.push_back(stSetting);
        stSetting.unIdentifier = H2_SETTINGS_MAX_CONCURRENT_STREAMS;
        stSetting.uiValue = 100;
        vecSetting.push_back(stSetting);
        m_pFrame->EncodeSetting(this, vecSetting, pBuff);
        m_pFrame->EncodeWindowUpdate(this, 0, 4128769, pBuff);
        // In HTTP/2, 1 on client is reserved for Upgrade.
        //m_uiStreamIdGenerate = 1;
    }
    else
    {
        stSetting.unIdentifier = H2_SETTINGS_INITIAL_WINDOW_SIZE;
        stSetting.uiValue = 4194304;
        m_uiRecvWindowSize = 4194304;
        m_uiSendWindowSize = 1048576;
        vecSetting.push_back(stSetting);
        stSetting.unIdentifier = H2_SETTINGS_MAX_FRAME_SIZE;
        stSetting.uiValue = 4194304;
        m_uiSettingsMaxDecodeFrameSize = 4194304;
        vecSetting.push_back(stSetting);
        stSetting.unIdentifier = H2_SETTINGS_MAX_HEADER_LIST_SIZE;
        stSetting.uiValue = 8192;
        m_uiSettingsMaxHeaderListSize = 8192;
        vecSetting.push_back(stSetting);
        m_pFrame->EncodeSetting(this, vecSetting, pBuff);
        m_pFrame->EncodeWindowUpdate(this, 0, 4128769, pBuff);
        m_pFrame->EncodePing(this, false, 0, 0, pBuff);
    }
}

E_CODEC_STATUS CodecHttp2::Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecHttp2::Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    if (m_bWantMagic && GetBindChannel()->IsClient())
    {
        pBuff->Write("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
        m_bWantMagic = false;
    }
    for (int i = 0; i < oHttpMsg.adding_without_index_headers_size(); ++i)
    {
        m_setEncodingWithoutIndexHeaders.insert(oHttpMsg.adding_without_index_headers(i));
    }
    for (int i = 0; i < oHttpMsg.adding_never_index_headers_size(); ++i)
    {
        m_setEncodingNeverIndexHeaders.insert(oHttpMsg.adding_never_index_headers(i));
    }
    if (oHttpMsg.settings_size() > 0)
    {
        std::vector<tagSetting> vecSetting;
        tagSetting stSetting;
        for (auto it = oHttpMsg.settings().begin(); it != oHttpMsg.settings().end(); ++it)
        {
            stSetting.unIdentifier = it->first;
            stSetting.uiValue = it->second;
            vecSetting.push_back(stSetting);
        }
        m_pFrame->EncodeSetting(this, vecSetting, pBuff);
    }
    if (HTTP_REQUEST == oHttpMsg.type())
    {
        if (oHttpMsg.stream_id() != 0)
        {
            LOG4_ERROR("request stream id must be zero.");
            return(CODEC_STATUS_PART_ERR);
        }
        const_cast<HttpMsg&>(oHttpMsg).set_stream_id(StreamIdGenerate());
    }
    else
    {
        if (oHttpMsg.stream_id() == 0)
        {
            LOG4_ERROR("response stream id can not be zero.");
            return(CODEC_STATUS_PART_ERR);
        }
    }
    //const_cast<HttpMsg&>(oHttpMsg).set_with_huffman(true);
    if (m_pCodingStream == nullptr || oHttpMsg.stream_id() != m_pCodingStream->GetStreamId())
    {
        auto stream_iter = m_mapStream.find(oHttpMsg.stream_id());
        if (stream_iter == m_mapStream.end())
        {
            if (NewCodingStream(oHttpMsg.stream_id()) == nullptr)
            {
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            m_pCodingStream = stream_iter->second;
        }
    }
    size_t uiReadIdx = pBuff->GetReadIndex();
    LOG4_TRACE("%s", oHttpMsg.DebugString().c_str());
    E_CODEC_STATUS eCodecStatus = m_pCodingStream->Encode(this, oHttpMsg, pBuff);
    if (CODEC_STATUS_PAUSE == eCodecStatus
            || CODEC_STATUS_ERR == eCodecStatus)
    {
        pBuff->SetReadIndex(uiReadIdx);
    }
    if (CODEC_STATUS_PART_ERR == eCodecStatus
            || CODEC_STATUS_OK == eCodecStatus)
    {
        LOG4_TRACE("is_client = %d, m_pCodingStream->GetStreamId() = %u",
                GetBindChannel()->IsClient(), m_pCodingStream->GetStreamId());
        if (m_pCodingStream->GetStreamState() == H2_STREAM_CLOSE)
        {
            CloseStream(m_stDecodeFrameHead.uiStreamIdentifier);
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecHttp2::Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(Encode(oHttpMsg, pBuff));
}

E_CODEC_STATUS CodecHttp2::Decode(CBuffer* pBuff, HttpMsg& oHttpMsg)
{
    LOG4_ERROR("invalid");
    return(CODEC_STATUS_ERR);
}

E_CODEC_STATUS CodecHttp2::Decode(CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u", pBuff->ReadableBytes());
    if (m_bWantMagic && !GetBindChannel()->IsClient())
    {
        if (pBuff->ReadableBytes() >= 24)
        {
            LOG4_TRACE("%s", pBuff->GetRawReadBuffer());
            std::string strMagic;
            strMagic.assign(pBuff->GetRawReadBuffer(), 24);
            if (strMagic == "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n")
            {
                m_bWantMagic = false;
                pBuff->AdvanceReadIndex(24);
                return(Decode(pBuff, oHttpMsg, pReactBuff));
            }
            LOG4_ERROR("need upgrade magic.");
            return(CODEC_STATUS_ERR);
        }
        return(CODEC_STATUS_PAUSE);
    }
    if (pBuff->ReadableBytes() < H2_FRAME_HEAD_SIZE)
    {
        if (pBuff->ReadableBytes() == 0 && m_pHoldingHttpMsg != nullptr)
        {
            oHttpMsg = std::move(*m_pHoldingHttpMsg);
            oHttpMsg.set_http_major(2);
            oHttpMsg.set_http_minor(0);
            oHttpMsg.mutable_upgrade()->set_is_upgrade(false);
            oHttpMsg.mutable_upgrade()->set_protocol("");
            oHttpMsg.set_stream_id(1);
            try
            {
                m_pCodingStream = new Http2Stream(m_pLogger, GetCodecType(), GetBindChannel(), oHttpMsg.stream_id());
                m_pCodingStream->SetState(H2_STREAM_HALF_CLOSE_REMOTE);
                m_mapStream.insert(std::make_pair((uint32)1, m_pCodingStream));
            }
            catch(std::bad_alloc& e)
            {
                LOG4_ERROR("%s", e.what());
            }
            delete m_pHoldingHttpMsg;
            m_pHoldingHttpMsg = nullptr;
            return(CODEC_STATUS_OK);
        }
        return(CODEC_STATUS_PAUSE);
    }

    size_t uiReadIdx = pBuff->GetReadIndex();
    Http2Frame::DecodeFrameHeader(pBuff, m_stDecodeFrameHead);
    LOG4_TRACE("m_stDecodeFrameHead.uiLength = %u, m_stDecodeFrameHead.ucType = %u,"
            "m_stDecodeFrameHead.ucFlag = %u, m_stDecodeFrameHead.uiStreamIdentifier = %u",
            m_stDecodeFrameHead.uiLength, (uint32)m_stDecodeFrameHead.ucType,
            (uint32)m_stDecodeFrameHead.ucFlag, m_stDecodeFrameHead.uiStreamIdentifier);
    if (!Http2Frame::IsValidFrameType(m_stDecodeFrameHead.ucType))
    {
        LOG4_ERROR("invalid frame type %u", (uint32)m_stDecodeFrameHead.ucType);
        SetErrno(H2_ERR_PROTOCOL_ERROR);
        m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "invalid frame type.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (m_stDecodeFrameHead.uiLength > m_uiSettingsMaxDecodeFrameSize)
    {
        LOG4_TRACE("m_uiSettingsMaxDecodeFrameSize = %u, m_stDecodeFrameHead.uiLength = %u",
                m_uiSettingsMaxDecodeFrameSize, m_stDecodeFrameHead.uiLength);
        SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "frame size is larger than max frame size.", pReactBuff);
        return(CODEC_STATUS_PART_ERR);
    }
    if (pBuff->ReadableBytes() < m_stDecodeFrameHead.uiLength)
    {
        LOG4_TRACE("pBuff->ReadableBytes() = %u, m_stDecodeFrameHead.uiLength = %u", pBuff->ReadableBytes(), m_stDecodeFrameHead.uiLength);
        pBuff->SetReadIndex(uiReadIdx);
        return(CODEC_STATUS_PAUSE);
    }

    if (m_uiGoawayLastStreamId > 0 && m_stDecodeFrameHead.uiStreamIdentifier > m_uiGoawayLastStreamId)
    {
        LOG4_TRACE("m_uiGoawayLastStreamId = %u, m_stDecodeFrameHead.uiStreamIdentifier = %u", m_uiGoawayLastStreamId, m_stDecodeFrameHead.uiStreamIdentifier);
        SetErrno(H2_ERR_CANCEL);
        pBuff->SetReadIndex(uiReadIdx);
        return(CODEC_STATUS_PART_ERR);
    }

    LOG4_TRACE("m_stDecodeFrameHead.uiStreamIdentifier = %u", m_stDecodeFrameHead.uiStreamIdentifier);
    if (m_stDecodeFrameHead.uiStreamIdentifier > 0)
    {
        if (m_pCodingStream != nullptr)
        {
            if (m_stDecodeFrameHead.uiStreamIdentifier == m_pCodingStream->GetStreamId())
            {
                E_CODEC_STATUS eCodecStatus = m_pCodingStream->Decode(this, m_stDecodeFrameHead, pBuff, oHttpMsg, pReactBuff);
                LOG4_TRACE("eCodecStatus = %d", eCodecStatus);
                if (CODEC_STATUS_PAUSE == eCodecStatus
                        || CODEC_STATUS_ERR == eCodecStatus)
                {
                    pBuff->SetReadIndex(uiReadIdx);
                }
                if (CODEC_STATUS_PART_ERR == eCodecStatus
                        || CODEC_STATUS_OK == eCodecStatus)
                {
                    if (m_pCodingStream->GetStreamState() == H2_STREAM_CLOSE)
                    {
                        CloseStream(m_stDecodeFrameHead.uiStreamIdentifier);
                    }
                }
                oHttpMsg.set_http_major(2);
                oHttpMsg.set_http_minor(0);
                return(eCodecStatus);
            }
        }
        if (H2_FRAME_CONTINUATION == m_stDecodeFrameHead.ucType)
        {
            SetErrno(H2_ERR_PROTOCOL_ERROR);
            m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "A CONTINUATION "
                    "frame MUST be preceded by a HEADERS, PUSH_PROMISE or "
                    "CONTINUATION frame without the END_HEADERS flag set. ", pReactBuff);
            LOG4_TRACE("The endpoint detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.");
            return(CODEC_STATUS_ERR);
        }
        LOG4_TRACE("m_stDecodeFrameHead.ucType = %u", (uint32)m_stDecodeFrameHead.ucType);
        auto stream_iter = m_mapStream.find(m_stDecodeFrameHead.uiStreamIdentifier);
        if (stream_iter == m_mapStream.end())
        {
            /** The identifier of a newly established stream MUST be numerically
             *  greater than all streams that the initiating endpoint has opened
             *  or reserved. This governs streams that are opened using a HEADERS
             *  frame and streams that are reserved using PUSH_PROMISE. An endpoint
             *  that receives an unexpected stream identifier MUST respond with
             *  a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
             */
            if (m_stDecodeFrameHead.uiStreamIdentifier <= m_uiStreamIdGenerate)
            {
                if (H2_FRAME_WINDOW_UPDATE == m_stDecodeFrameHead.ucType
                        || H2_FRAME_RST_STREAM == m_stDecodeFrameHead.ucType
                        || H2_FRAME_PRIORITY == m_stDecodeFrameHead.ucType)
                {
                    E_CODEC_STATUS eCodecStatus = m_pFrame->Decode(this, m_stDecodeFrameHead, pBuff, oHttpMsg, pReactBuff);
                    if (CODEC_STATUS_PAUSE == eCodecStatus
                            || CODEC_STATUS_ERR == eCodecStatus)
                    {
                        pBuff->SetReadIndex(uiReadIdx);
                    }
                    oHttpMsg.set_http_major(2);
                    oHttpMsg.set_http_minor(0);
                    return(eCodecStatus);
                }
                else
                {
                    SetErrno(H2_ERR_PROTOCOL_ERROR);
                    m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "The "
                            "identifier of a newly established stream MUST be "
                            "numerically greater than all streams that the "
                            "initiating endpoint has opened or reserved.", pReactBuff);
                    return(CODEC_STATUS_ERR);
                }
            }
            m_uiStreamIdGenerate = m_stDecodeFrameHead.uiStreamIdentifier;
            if (NewCodingStream(m_stDecodeFrameHead.uiStreamIdentifier) == nullptr)
            {
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            m_pCodingStream = stream_iter->second;
        }
        E_CODEC_STATUS eCodecStatus = m_pCodingStream->Decode(this, m_stDecodeFrameHead, pBuff, oHttpMsg, pReactBuff);
        if (CODEC_STATUS_PAUSE == eCodecStatus
                || CODEC_STATUS_ERR == eCodecStatus)
        {
            pBuff->SetReadIndex(uiReadIdx);
        }
        if (CODEC_STATUS_PART_ERR == eCodecStatus
                || CODEC_STATUS_OK == eCodecStatus)
        {
            if (m_pCodingStream->GetStreamState() == H2_STREAM_CLOSE)
            {
                CloseStream(m_stDecodeFrameHead.uiStreamIdentifier);
            }
        }
        oHttpMsg.set_http_major(2);
        oHttpMsg.set_http_minor(0);
        return(eCodecStatus);
    }
    else
    {
        E_CODEC_STATUS eCodecStatus = m_pFrame->Decode(this, m_stDecodeFrameHead, pBuff, oHttpMsg, pReactBuff);
        if (CODEC_STATUS_PAUSE == eCodecStatus
                || CODEC_STATUS_ERR == eCodecStatus)
        {
            pBuff->SetReadIndex(uiReadIdx);
        }
        oHttpMsg.set_http_major(2);
        oHttpMsg.set_http_minor(0);
        return(eCodecStatus);
    }
}

void CodecHttp2::SetPriority(uint32 uiStreamId, const tagPriority& stPriority)
{
    auto pCurrentStreamWeight = FindStreamWeight(uiStreamId, m_pStreamWeightRoot);
    if (pCurrentStreamWeight == nullptr)
    {
        try
        {
            pCurrentStreamWeight = new TreeNode<tagStreamWeight>();
            pCurrentStreamWeight->pData = new tagStreamWeight();
            pCurrentStreamWeight->pData->uiStreamId = uiStreamId;
            pCurrentStreamWeight->pData->ucWeight = stPriority.ucWeight;
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("%s", e.what());
            return;
        }
    }
    if (pCurrentStreamWeight->pParent != nullptr)
    {
        if (pCurrentStreamWeight->pParent->pFirstChild == pCurrentStreamWeight)
        {
            pCurrentStreamWeight->pParent->pFirstChild = pCurrentStreamWeight->pRightBrother;
        }
        else
        {
            auto pLast = pCurrentStreamWeight->pParent->pFirstChild;
            while (pLast->pRightBrother != pCurrentStreamWeight)
            {
                pLast = pLast->pRightBrother;
            }
            pLast->pRightBrother = pCurrentStreamWeight->pRightBrother;
        }
    }
    auto pDependencyStreamWeight = FindStreamWeight(stPriority.uiDependency, m_pStreamWeightRoot);
    if (pDependencyStreamWeight == nullptr)
    {
        pCurrentStreamWeight->pRightBrother = m_pStreamWeightRoot->pRightBrother;
        m_pStreamWeightRoot->pRightBrother = pCurrentStreamWeight;
    }
    else
    {
        if (stPriority.E)
        {
            pCurrentStreamWeight->pFirstChild = pDependencyStreamWeight->pFirstChild;
        }
        else
        {
            pCurrentStreamWeight->pRightBrother = pDependencyStreamWeight->pFirstChild;
        }
        pDependencyStreamWeight->pFirstChild = pCurrentStreamWeight;
        pCurrentStreamWeight->pParent = pDependencyStreamWeight;
    }
}

void CodecHttp2::RstStream(uint32 uiStreamId)
{
    auto iter = m_mapStream.find(uiStreamId);
    if (iter != m_mapStream.end())
    {
        auto pStreamWeight = FindStreamWeight(uiStreamId, m_pStreamWeightRoot);
        if (pStreamWeight != nullptr)
        {
            auto pParent = pStreamWeight->pParent;
            if (pParent != nullptr)
            {
                if (pParent->pFirstChild == pStreamWeight)
                {
                    if (pStreamWeight->pFirstChild != nullptr)
                    {
                        pParent->pFirstChild = pStreamWeight->pFirstChild;
                    }
                    else
                    {
                        pParent->pFirstChild = pStreamWeight->pRightBrother;
                    }
                }
                else
                {
                    auto pBrother = pParent->pFirstChild;
                    while (pBrother->pRightBrother != pStreamWeight)
                    {
                        pBrother = pBrother->pRightBrother;
                    }
                    pBrother->pRightBrother = pStreamWeight->pRightBrother;
                }
                delete pStreamWeight;
            }
        }
        else
        {
            // TODO?
        }

        if (iter->second == m_pCodingStream)
        {
            m_pCodingStream = nullptr;
        }
        delete iter->second;
        iter->second = nullptr;
        m_mapStream.erase(iter);
    }
}

E_H2_ERR_CODE CodecHttp2::Setting(const std::vector<tagSetting>& vecSetting, bool bRecvSetting)
{
    for (size_t i = 0; i < vecSetting.size(); ++i)
    {
        switch (vecSetting[i].unIdentifier)
        {
            case H2_SETTINGS_HEADER_TABLE_SIZE:
                if (vecSetting[i].uiValue <= SETTINGS_MAX_FRAME_SIZE)
                {
                    m_uiSettingsHeaderTableSize = vecSetting[i].uiValue;
                }
                break;
            case H2_SETTINGS_ENABLE_PUSH:
                if (vecSetting[i].uiValue == 0 || vecSetting[i].uiValue == 1)
                {
                    m_uiSettingsEnablePush = vecSetting[i].uiValue;
                }
                else
                {
                    return(H2_ERR_PROTOCOL_ERROR);
                }
                break;
            case H2_SETTINGS_MAX_CONCURRENT_STREAMS:
                m_uiSettingsMaxConcurrentStreams = vecSetting[i].uiValue;
                break;
            case H2_SETTINGS_INITIAL_WINDOW_SIZE:
                if (vecSetting[i].uiValue <= SETTINGS_MAX_INITIAL_WINDOW_SIZE)
                {
                    for (auto it = m_mapStream.begin(); it != m_mapStream.end(); ++it)
                    {
                        it->second->WindowInit(vecSetting[i].uiValue - m_uiSettingsMaxWindowSize);
                    }
                    m_uiSettingsMaxWindowSize = vecSetting[i].uiValue;
                }
                else
                {
                    return(H2_ERR_FLOW_CONTROL_ERROR);
                }
                break;
            case H2_SETTINGS_MAX_FRAME_SIZE:
                if (vecSetting[i].uiValue <= SETTINGS_MAX_FRAME_SIZE)
                {
                    if (bRecvSetting)
                    {
                        m_uiSettingsMaxEncodeFrameSize = vecSetting[i].uiValue;
                        LOG4_TRACE("set max frame size to %u", m_uiSettingsMaxEncodeFrameSize);
                    }
                    else
                    {
                        m_uiSettingsMaxDecodeFrameSize = vecSetting[i].uiValue;
                    }
                }
                else
                {
                    return(H2_ERR_PROTOCOL_ERROR);
                }
                break;
            case H2_SETTINGS_MAX_HEADER_LIST_SIZE:
                m_uiSettingsMaxHeaderListSize = vecSetting[i].uiValue;
                break;
            default:
                ;   // undefine setting, ignore
        }
    }
    return(H2_ERR_NO_ERROR);
}

void CodecHttp2::WindowUpdate(uint32 uiStreamId, uint32 uiIncrement)
{
    m_uiSendWindowSize += uiIncrement;
    if (uiStreamId > 0)
    {
        auto iter = m_mapStream.find(uiStreamId);
        if (iter != m_mapStream.end())
        {
            iter->second->WindowUpdate((int32)uiIncrement);
        }
    }
}

void CodecHttp2::UpdateSendWindow(uint32 uiStreamId, uint32 uiSendLength)
{
    m_uiSendWindowSize -= uiSendLength;
    if (uiStreamId > 0)
    {
        auto iter = m_mapStream.find(uiStreamId);
        if (iter != m_mapStream.end())
        {
            iter->second->WindowUpdate(-uiSendLength);
        }
    }
}

void CodecHttp2::UpdateRecvWindow(uint32 uiStreamId, uint32 uiRecvLength, CBuffer* pBuff)
{
    m_uiRecvWindowSize -= uiRecvLength;
    if (m_uiRecvWindowSize < DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE / 4)
    {
        m_pFrame->EncodeWindowUpdate(this, 0,
                SETTINGS_MAX_INITIAL_WINDOW_SIZE - uiRecvLength, pBuff);
        m_uiRecvWindowSize = SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    }
    if (uiStreamId > 0)
    {
        auto iter = m_mapStream.find(uiStreamId);
        if (iter != m_mapStream.end())
        {
            iter->second->UpdateRecvWindow(this, uiStreamId, uiRecvLength, pBuff);
        }
    }
}

E_CODEC_STATUS CodecHttp2::UnpackHeader(uint32 uiHeaderBlockEndPos, CBuffer* pBuff, HttpMsg& oHttpMsg)
{
    char B = 0;
    size_t uiReadIndex = 0;

    E_CODEC_STATUS eStatus;
    bool bWithHuffman = false;
    std::string strHeaderName;
    std::string strHeaderValue;
    while (pBuff->GetReadIndex() < uiHeaderBlockEndPos)
    {
        uiReadIndex = pBuff->GetReadIndex();
        pBuff->ReadByte(B);
        pBuff->SetReadIndex(uiReadIndex);
        LOG4_TRACE("B = 0x%X", (uint32)B);
        if (H2_HPACK_CONDITION_INDEXED_HEADER & B)
        {
            eStatus = UnpackHeaderIndexed(pBuff, oHttpMsg);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
        }
        else if (H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING & B)
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_6_BITS,
                    strHeaderName, strHeaderValue, bWithHuffman);
            ClassifyHeader(strHeaderName, strHeaderValue, oHttpMsg);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
        }
        else if (H2_HPACK_CONDITION_LITERAL_HEADER_NEVER_INDEXED & B)
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_4_BITS,
                    strHeaderName, strHeaderValue, bWithHuffman);
            ClassifyHeader(strHeaderName, strHeaderValue, oHttpMsg);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
            oHttpMsg.add_adding_never_index_headers(strHeaderName);
        }
        else if (H2_HPACK_CONDITION_DYNAMIC_TABLE_SIZE_UPDATE & B)
        {
            uint32 uiTableSize = (uint32)Http2Header::DecodeInt(H2_HPACK_PREFIX_5_BITS, pBuff);
            LOG4_TRACE("uiTableSize = %u", uiTableSize);
            if (uiTableSize > m_uiSettingsHeaderTableSize)
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("The new maximum size MUST be lower than or equal to "
                        "the limit(SETTINGS_HEADER_TABLE_SIZE) determined by the"
                        " protocol using HPACK!");
                return(CODEC_STATUS_ERR);
            }
            UpdateDecodingDynamicTable(uiTableSize);
        }
        else    // H2_HPACK_CONDITION_LITERAL_HEADER_WITHOUT_INDEXING
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_4_BITS,
                    strHeaderName, strHeaderValue, bWithHuffman);
            ClassifyHeader(strHeaderName, strHeaderValue, oHttpMsg);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
            oHttpMsg.add_adding_without_index_headers(strHeaderName);
        }
    }
    oHttpMsg.set_with_huffman(bWithHuffman);
    return(CODEC_STATUS_PART_OK);
}

void CodecHttp2::PackHeader(const HttpMsg& oHttpMsg, int iHeaderType, CBuffer* pBuff)
{
    if (oHttpMsg.dynamic_table_update_size() > 0)
    {
        if (oHttpMsg.dynamic_table_update_size() > SETTINGS_MAX_FRAME_SIZE)
        {
            LOG4_WARNING("invalid dynamic table update size %u, the size must smaller than %u.",
                    oHttpMsg.dynamic_table_update_size(), SETTINGS_MAX_FRAME_SIZE);
        }
        else
        {
            PackHeaderDynamicTableSize(oHttpMsg.dynamic_table_update_size(), pBuff);
        }
    }

    if (iHeaderType & H2_HEADER_PSEUDO)
    {
        for (int i = 0; i < oHttpMsg.pseudo_header_size(); ++i)
        {
            PackHeader(oHttpMsg.pseudo_header(i).name(), oHttpMsg.pseudo_header(i).value(), oHttpMsg.with_huffman(), pBuff);
        }
    }
    if (iHeaderType & H2_HEADER_NORMAL)
    {
        std::string strHeaderName;
        for (auto c_iter = oHttpMsg.headers().begin();
                c_iter != oHttpMsg.headers().end(); ++c_iter)
        {
            strHeaderName = c_iter->first;
            std::transform(strHeaderName.begin(), strHeaderName.end(), strHeaderName.begin(),
                    [](unsigned char c) -> unsigned char { return std::tolower(c); });
            PackHeader(strHeaderName, c_iter->second, oHttpMsg.with_huffman(), pBuff);
        }
    }
    if (iHeaderType & H2_HEADER_TRAILER)
    {
        for (int i = 0; i < oHttpMsg.trailer_header_size(); ++i)
        {
            PackHeader(oHttpMsg.trailer_header(i).name(), oHttpMsg.trailer_header(i).value(), oHttpMsg.with_huffman(), pBuff);
        }
    }
}

void CodecHttp2::PackHeader(const std::string& strHeaderName, const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff)
{
    uint32 uiHeaderIndex = 0;
    uint32 uiHeaderNameIndex = 0;
    uiHeaderIndex = GetEncodingTableIndex(strHeaderName, strHeaderValue, uiHeaderNameIndex);
    if (uiHeaderIndex > 0)
    {
        PackHeaderIndexed(uiHeaderIndex, pBuff);
    }
    else
    {
        auto never_index_iter = m_setEncodingNeverIndexHeaders.find(strHeaderName);
        if (never_index_iter != m_setEncodingNeverIndexHeaders.end())
        {
            PackHeaderNeverIndexing(strHeaderName, strHeaderValue, uiHeaderNameIndex, bWithHuffman, pBuff);
            return;
        }
        auto without_index_iter = m_setEncodingWithoutIndexHeaders.find(strHeaderName);
        if (without_index_iter != m_setEncodingWithoutIndexHeaders.end())
        {
            PackHeaderWithoutIndexing(strHeaderName, strHeaderValue, uiHeaderNameIndex, bWithHuffman, pBuff);
            return;
        }
        PackHeaderWithIndexing(strHeaderName, strHeaderValue, uiHeaderNameIndex, bWithHuffman, pBuff);
    }
}

// TODO 在另一个 stream 流上发送 PUSH_PROMISE 帧保留了用于以后使用的空闲流。保留 stream 流的流状态转换为 "reserved (local)" 保留(本地)状态。
E_CODEC_STATUS CodecHttp2::PromiseStream(uint32 uiStreamId, CBuffer* pReactBuff)
{
    if (m_uiGoawayLastStreamId > 0 && uiStreamId > m_uiGoawayLastStreamId)
    {
        SetErrno(H2_ERR_INTERNAL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    /** The identifier of a newly established stream MUST be numerically
     *  greater than all streams that the initiating endpoint has opened
     *  or reserved. This governs streams that are opened using a HEADERS
     *  frame and streams that are reserved using PUSH_PROMISE. An endpoint
     *  that receives an unexpected stream identifier MUST respond with
     *  a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
     */
    if (uiStreamId <= m_uiStreamIdGenerate)
    {
        SetErrno(H2_ERR_REFUSED_STREAM);
        m_pFrame->EncodeRstStream(this, uiStreamId, H2_ERR_REFUSED_STREAM, pReactBuff);
        return(CODEC_STATUS_PART_ERR);
    }
    m_uiStreamIdGenerate = uiStreamId;
    Http2Stream* pPromiseStream = nullptr;
    try
    {
        pPromiseStream = new Http2Stream(m_pLogger, GetCodecType(), GetBindChannel(), uiStreamId);
        pPromiseStream->SetState(H2_STREAM_RESERVED_REMOTE);
        m_mapStream.insert(std::make_pair(uiStreamId, pPromiseStream));
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
        return(CODEC_STATUS_PART_ERR);
    }
    return(CODEC_STATUS_PART_OK);
}

E_CODEC_STATUS CodecHttp2::SendWaittingFrameData(CBuffer* pBuff)
{
    if (m_bHasWaittingFrame && m_pStreamWeightRoot != nullptr)
    {
        E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
        uint32 uiStreamId = 0;
        std::vector<uint32> vecCompletedStream;
        auto pCurrent = m_pStreamWeightRoot->pFirstChild;
        while (pCurrent)
        {
            uiStreamId = pCurrent->pData->uiStreamId;
            auto iter = m_mapStream.find(uiStreamId);
            if (iter != m_mapStream.end())
            {
                eStatus = iter->second->SendWaittingFrameData(this, pBuff);
                if (iter->second->GetStreamState() == H2_STREAM_CLOSE)
                {
                    vecCompletedStream.push_back(uiStreamId);
                }
            }
            if (pCurrent->pFirstChild != nullptr)
            {
                pCurrent = pCurrent->pFirstChild;
            }
            else
            {
                pCurrent = pCurrent->pRightBrother;
            }
        }
        for (auto id : vecCompletedStream)
        {
            CloseStream(id);
        }
        return(eStatus);
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecHttp2::SendWaittingFrameData(
        TreeNode<tagStreamWeight>* pStreamWeight,
        std::vector<uint32>& vecCompletedStream, CBuffer* pBuff)
{
    return(CODEC_STATUS_OK);
}

void CodecHttp2::TransferHoldingMsg(HttpMsg* pHoldingHttpMsg)
{
    m_pHoldingHttpMsg = pHoldingHttpMsg;
}

uint32 CodecHttp2::StreamIdGenerate()
{
    if (GetBindChannel()->IsClient())
    {
        if (m_uiStreamIdGenerate & 0x01)    // odd number
        {
            m_uiStreamIdGenerate = (m_uiStreamIdGenerate + 2) & STREAM_IDENTIFY_MASK;
        }
        else
        {
            m_uiStreamIdGenerate = (m_uiStreamIdGenerate + 1) & STREAM_IDENTIFY_MASK;
        }
    }
    else
    {
        if (m_uiStreamIdGenerate & 0x01)
        {
            m_uiStreamIdGenerate = (m_uiStreamIdGenerate + 1) & STREAM_IDENTIFY_MASK;
        }
        else
        {
            m_uiStreamIdGenerate = (m_uiStreamIdGenerate + 2) & STREAM_IDENTIFY_MASK;
        }
    }
    return(m_uiStreamIdGenerate);
}

Http2Stream* CodecHttp2::NewCodingStream(uint32 uiStreamId)
{
    uint32 uiNewWindowSize = m_uiRecvWindowSize + DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE;
    try
    {
        m_pCodingStream = new Http2Stream(m_pLogger, GetCodecType(), GetBindChannel(), uiStreamId);
        m_pCodingStream->WindowInit(m_uiSettingsMaxWindowSize);
        m_uiRecvWindowSize = (uiNewWindowSize < SETTINGS_MAX_INITIAL_WINDOW_SIZE)
                ? uiNewWindowSize : SETTINGS_MAX_INITIAL_WINDOW_SIZE;
        m_mapStream.insert(std::make_pair(uiStreamId, m_pCodingStream));
        return(m_pCodingStream);
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
        return(nullptr);
    }
}

TreeNode<tagStreamWeight>* CodecHttp2::FindStreamWeight(uint32 uiStreamId, TreeNode<tagStreamWeight>* pTarget)
{
    if (pTarget == nullptr)
    {
        return(nullptr);
    }
    if (pTarget->pData->uiStreamId == uiStreamId)
    {
        return(pTarget);
    }
    else
    {
        auto pFound = FindStreamWeight(uiStreamId, pTarget->pFirstChild);
        if (pFound == nullptr)
        {
            pFound = FindStreamWeight(uiStreamId, pTarget->pRightBrother);
        }
        return(pFound);
    }
}

void CodecHttp2::ReleaseStreamWeight(TreeNode<tagStreamWeight>* pNode)
{
    if (pNode == nullptr)
    {
        return;
    }
    ReleaseStreamWeight(pNode->pFirstChild);
    pNode->pFirstChild = nullptr;
    ReleaseStreamWeight(pNode->pRightBrother);
    pNode->pRightBrother = nullptr;
    pNode->pParent = nullptr;
    delete pNode->pData;
    pNode->pData = nullptr;
    delete pNode;
}

E_CODEC_STATUS CodecHttp2::UnpackHeaderIndexed(CBuffer* pBuff, HttpMsg& oHttpMsg)
{
    uint32 uiTableIndex = (uint32)Http2Header::DecodeInt(H2_HPACK_PREFIX_7_BITS, pBuff);
    LOG4_TRACE("uiTableIndex = %u", uiTableIndex);
    if (uiTableIndex == 0)
    {
        SetErrno(H2_ERR_COMPRESSION_ERROR);
        LOG4_ERROR("hpack index value of 0 is not used!");
        return(CODEC_STATUS_ERR);
    }
    else if (uiTableIndex < Http2Header::sc_uiMaxStaticTableLength)
    {
        ClassifyHeader(Http2Header::sc_vecStaticTable[uiTableIndex].first,
                Http2Header::sc_vecStaticTable[uiTableIndex].second, oHttpMsg);
        return(CODEC_STATUS_PART_OK);
    }
    else
    {
        uint32 uiDynamicTableIndex = DecodingDynamicTableIndex2VectorIndex(uiTableIndex - Http2Header::sc_uiMaxStaticTableLength);
        if (uiDynamicTableIndex < m_vecDecodingDynamicTable.size())
        {
            ClassifyHeader(m_vecDecodingDynamicTable[uiDynamicTableIndex].Name(),
                    m_vecDecodingDynamicTable[uiDynamicTableIndex].Value(), oHttpMsg);
            return(CODEC_STATUS_PART_OK);
        }
        else
        {
            SetErrno(H2_ERR_COMPRESSION_ERROR);
            LOG4_ERROR("hpack index value of %u was greater than the sum of "
                    "the lengths of both static table and dynamic tables!", uiTableIndex);
            return(CODEC_STATUS_ERR);
        }
    }
}

E_CODEC_STATUS CodecHttp2::UnpackHeaderLiteralIndexing(CBuffer* pBuff, uint8 ucFirstByte, int32 iPrefixMask,
        std::string& strHeaderName, std::string& strHeaderValue, bool& bWithHuffman)
{
    strHeaderName.clear();
    strHeaderValue.clear();
    bWithHuffman = false;
    // Literal Header Field with Incremental Indexing — Indexed Name
    if (iPrefixMask & ucFirstByte)
    {
        uint32 uiTableIndex = (uint32)Http2Header::DecodeInt(iPrefixMask, pBuff);
        LOG4_TRACE("uiTableIndex = %u", uiTableIndex);
        LOG4_TRACE("Literal Header Field with Incremental Indexing — Indexed Name");
        if (uiTableIndex == 0)
        {
            SetErrno(H2_ERR_COMPRESSION_ERROR);
            LOG4_ERROR("hpack index value of 0 is not used!");
            return(CODEC_STATUS_ERR);
        }
        else if (uiTableIndex < Http2Header::sc_uiMaxStaticTableLength)
        {
            LOG4_TRACE("uiTableIndex < Http2Header::sc_uiMaxStaticTableLength");
            if (!Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("DecodeStringLiteral failed!");
                return(CODEC_STATUS_ERR);
            }
            strHeaderName = Http2Header::sc_vecStaticTable[uiTableIndex].first;
            if (H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING & ucFirstByte)
            {
                UpdateDecodingDynamicTable(-1, strHeaderName, strHeaderValue);
            }
            return(CODEC_STATUS_PART_OK);
        }
        else
        {
            uint32 uiVectorIndex = DecodingDynamicTableIndex2VectorIndex(uiTableIndex - Http2Header::sc_uiMaxStaticTableLength);
            if (uiVectorIndex < m_vecDecodingDynamicTable.size())
            {
                if (!Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
                {
                    SetErrno(H2_ERR_COMPRESSION_ERROR);
                    LOG4_ERROR("DecodeStringLiteral failed!");
                    return(CODEC_STATUS_ERR);
                }
                strHeaderName = m_vecDecodingDynamicTable[uiVectorIndex].Name();
                if (H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING & ucFirstByte)
                {
                    UpdateDecodingDynamicTable(uiVectorIndex, strHeaderName, strHeaderValue);
                }
                return(CODEC_STATUS_PART_OK);
            }
            else
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("hpack index value of %u was greater than the sum of "
                        "the lengths of both static table and dynamic tables!", uiTableIndex);
                return(CODEC_STATUS_ERR);
            }
        }
    }
    else  // Literal Header Field with Incremental Indexing — New Name
    {
        LOG4_TRACE("Literal Header Field with Incremental Indexing — New Name");
        pBuff->SkipBytes(1);
        if (Http2Header::DecodeStringLiteral(pBuff, strHeaderName, bWithHuffman)
            && Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
        {
            if (H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING & ucFirstByte)
            {
                UpdateDecodingDynamicTable(-1, strHeaderName, strHeaderValue);
            }
            return(CODEC_STATUS_PART_OK);
        }
        SetErrno(H2_ERR_COMPRESSION_ERROR);
        LOG4_ERROR("DecodeStringLiteral header name or header value failed!");
        return(CODEC_STATUS_ERR);
    }
}

void CodecHttp2::ClassifyHeader(const std::string& strHeaderName, const std::string& strHeaderValue, HttpMsg& oHttpMsg)
{
    LOG4_TRACE("strHeaderName = %s, strHeaderValue = %s", strHeaderName.c_str(), strHeaderValue.c_str());
    if (GetBindChannel()->IsClient())
    {
        if (oHttpMsg.stream_id() & 0x01)
        {
            if (oHttpMsg.body().size() > 0)
            {
                auto pHeader = oHttpMsg.add_trailer_header();
                pHeader->set_name(strHeaderName);
                pHeader->set_value(strHeaderValue);
            }
            else
            {
                if (strHeaderName == ":status")
                {
                    oHttpMsg.set_status_code(StringConverter::RapidAtoi<int32>(strHeaderValue.c_str()));
                }
                else
                {
                    oHttpMsg.mutable_headers()->insert({strHeaderName, strHeaderValue});
                }
            }
        }
        else
        {
            if (oHttpMsg.body().size() > 0)
            {
                auto pHeader = oHttpMsg.add_trailer_header();
                pHeader->set_name(strHeaderName);
                pHeader->set_value(strHeaderValue);
            }
            else
            {
                if (strHeaderName == ":method")
                {
                    if (strHeaderValue == "POST")
                    {
                        oHttpMsg.set_method(HTTP_POST);
                    }
                    else if (strHeaderValue == "GET")
                    {
                        oHttpMsg.set_method(HTTP_GET);
                    }
                    else
                    {
                        ;// TODO other http method
                    }
                }
                else if (strHeaderName == ":path")
                {
                    oHttpMsg.set_path(strHeaderValue);
                }
                else
                {
                    oHttpMsg.mutable_headers()->insert({strHeaderName, strHeaderValue});
                }
            }
        }
    }
    else
    {
        if (oHttpMsg.stream_id() & 0x01)
        {
            if (oHttpMsg.body().size() > 0)
            {
                auto pHeader = oHttpMsg.add_trailer_header();
                pHeader->set_name(strHeaderName);
                pHeader->set_value(strHeaderValue);
            }
            else
            {
                if (strHeaderName == ":method")
                {
                    if (strHeaderValue == "POST")
                    {
                        oHttpMsg.set_method(HTTP_POST);
                    }
                    else if (strHeaderValue == "GET")
                    {
                        oHttpMsg.set_method(HTTP_GET);
                    }
                    else
                    {
                        ;// TODO other http method
                    }
                }
                else if (strHeaderName == ":path")
                {
                    oHttpMsg.set_path(strHeaderValue);
                }
                else
                {
                    oHttpMsg.mutable_headers()->insert({strHeaderName, strHeaderValue});
                }
            }
        }
        else
        {
            if (oHttpMsg.body().size() > 0)
            {
                auto pHeader = oHttpMsg.add_trailer_header();
                pHeader->set_name(strHeaderName);
                pHeader->set_value(strHeaderValue);
            }
            else
            {
                if (strHeaderName == ":status")
                {
                    oHttpMsg.set_status_code(StringConverter::RapidAtoi<int32>(strHeaderValue.c_str()));
                }
                else
                {
                    oHttpMsg.mutable_headers()->insert({strHeaderName, strHeaderValue});
                }
            }
        }
    }
}

void CodecHttp2::PackHeaderIndexed(size_t uiTableIndex, CBuffer* pBuff)
{
    Http2Header::EncodeInt(uiTableIndex, (size_t)H2_HPACK_PREFIX_7_BITS,
            (char)H2_HPACK_CONDITION_INDEXED_HEADER, pBuff);
}

void CodecHttp2::PackHeaderWithIndexing(const std::string& strHeaderName,
        const std::string& strHeaderValue, uint32 uiHeaderNameIndex,
        bool bWithHuffman, CBuffer* pBuff)
{
    if (uiHeaderNameIndex > 0)   // Literal Header Field with Incremental Indexing - Indexed Name.
    {
        Http2Header::EncodeInt(uiHeaderNameIndex, (size_t)H2_HPACK_PREFIX_6_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
        UpdateEncodingDynamicTable(strHeaderName, strHeaderValue);
    }
    else    // Literal Header Field with Incremental Indexing - New Name.
    {
        Http2Header::EncodeInt(0, (size_t)H2_HPACK_PREFIX_6_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
        UpdateEncodingDynamicTable(strHeaderName, strHeaderValue);
    }
}

void CodecHttp2::PackHeaderWithoutIndexing(const std::string& strHeaderName,
        const std::string& strHeaderValue,uint32 uiHeaderNameIndex,
        bool bWithHuffman, CBuffer* pBuff)
{
    if (uiHeaderNameIndex > 0)
    {
        Http2Header::EncodeInt(uiHeaderNameIndex, (size_t)H2_HPACK_PREFIX_4_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_WITHOUT_INDEXING, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
    }
    else
    {
        Http2Header::EncodeInt(0, (size_t)H2_HPACK_PREFIX_4_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_WITHOUT_INDEXING, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
    }
}

void CodecHttp2::PackHeaderNeverIndexing(const std::string& strHeaderName,
        const std::string& strHeaderValue, uint32 uiHeaderNameIndex,
        bool bWithHuffman, CBuffer* pBuff)
{
    if (uiHeaderNameIndex > 0)
    {
        Http2Header::EncodeInt(uiHeaderNameIndex, (size_t)H2_HPACK_PREFIX_4_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_NEVER_INDEXED, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
    }
    else
    {
        Http2Header::EncodeInt(0, (size_t)H2_HPACK_PREFIX_4_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_NEVER_INDEXED, pBuff);
        if (bWithHuffman)
        {
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteralWithHuffman(strHeaderValue, pBuff);
        }
        else
        {
            Http2Header::EncodeStringLiteral(strHeaderName, pBuff);
            Http2Header::EncodeStringLiteral(strHeaderValue, pBuff);
        }
    }
}

void CodecHttp2::PackHeaderDynamicTableSize(uint32 uiDynamicTableSize, CBuffer* pBuff)
{
    Http2Header::EncodeInt(uiDynamicTableSize, (size_t)H2_HPACK_PREFIX_5_BITS,
            (char)H2_HPACK_CONDITION_DYNAMIC_TABLE_SIZE_UPDATE, pBuff);
}

uint32 CodecHttp2::GetEncodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue, uint32& uiNameIndex)
{
    uiNameIndex = 0;
    uint32 uiTableIndex = 0;
    uiTableIndex = Http2Header::GetStaticTableIndex(strHeaderName, strHeaderValue, uiNameIndex);
    if (uiTableIndex > 0)
    {
        return(uiTableIndex);
    }
    int iEncodingDynamicTableLength = (int)m_vecEncodingDynamicTable.size();
    for (int i = m_iNextEncodingDynamicTableHeaderIndex + 1; i < iEncodingDynamicTableLength; ++i)
    {
        if (m_vecEncodingDynamicTable[i].Name() == strHeaderName)
        {
            if (m_vecEncodingDynamicTable[i].Value() == strHeaderValue)
            {
                uiTableIndex = i - m_iNextEncodingDynamicTableHeaderIndex + Http2Header::sc_uiMaxStaticTableIndex;
                break;
            }
            else if (uiNameIndex == 0)
            {
                uiNameIndex = i - m_iNextEncodingDynamicTableHeaderIndex + Http2Header::sc_uiMaxStaticTableIndex;
            }
        }
    }
    return(uiTableIndex);
}

uint32 CodecHttp2::GetDecodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue, uint32& uiNameIndex)
{
    uiNameIndex = 0;
    uint32 uiTableIndex = 0;
    uiTableIndex = Http2Header::GetStaticTableIndex(strHeaderName, strHeaderValue, uiNameIndex);
    if (uiTableIndex > 0)
    {
        return(uiTableIndex);
    }
    int iDecodingDynamicTableLength = (int)m_vecDecodingDynamicTable.size();
    for (int i = m_iNextDecodingDynamicTableHeaderIndex + 1; i < iDecodingDynamicTableLength; ++i)
    {
        if (m_vecDecodingDynamicTable[i].Name() == strHeaderName)
        {
            if (m_vecDecodingDynamicTable[i].Value() == strHeaderValue)
            {
                uiTableIndex = i - m_iNextDecodingDynamicTableHeaderIndex + Http2Header::sc_uiMaxStaticTableIndex;
                break;
            }
            else if (uiNameIndex == 0)
            {
                uiNameIndex = i - m_iNextDecodingDynamicTableHeaderIndex + Http2Header::sc_uiMaxStaticTableIndex;
            }
        }
    }
    return(uiTableIndex);
}

void CodecHttp2::UpdateEncodingDynamicTable(const std::string& strHeaderName, const std::string& strHeaderValue)
{
    Http2Header oHeader(strHeaderName, strHeaderValue);
    uint32 uiEntrySize = oHeader.HpackSize();
    // if the new header is too big, drop all entries
    if (uiEntrySize > m_uiSettingsHeaderTableSize)
    {
        m_uiEncodingDynamicTableSize = 0;
        m_uiEncodingDynamicTableHeaderCount = 0;
        m_iNextEncodingDynamicTableHeaderIndex = m_vecEncodingDynamicTable.size() - 1;
        return;
    }

    // evict headers to required length
    int32 iNeedRecoverSize = (m_uiEncodingDynamicTableSize + uiEntrySize) - m_uiSettingsHeaderTableSize;
    UpdateEncodingDynamicTable(iNeedRecoverSize);

    if (m_uiEncodingDynamicTableHeaderCount + 1 > m_vecEncodingDynamicTable.size()) // need to grow the dynamic table.
    {
        Http2Header oHeader("", "");
        std::vector<Http2Header> vecDynamicTable;
        vecDynamicTable.assign(m_vecEncodingDynamicTable.size() * 2, oHeader);
        for (uint32 i = 0; i < m_vecEncodingDynamicTable.size(); ++i)
        {
            vecDynamicTable[m_vecEncodingDynamicTable.size() + i] = std::move(m_vecEncodingDynamicTable[i]);
        }
        m_iNextEncodingDynamicTableHeaderIndex = m_vecEncodingDynamicTable.size() - 1;
        m_vecEncodingDynamicTable = std::move(vecDynamicTable);
    }
    int iCurrent = m_iNextEncodingDynamicTableHeaderIndex--;
    m_vecEncodingDynamicTable[iCurrent] = std::move(oHeader);
    ++m_uiEncodingDynamicTableHeaderCount;
    m_uiEncodingDynamicTableSize += uiEntrySize;
}

uint32 CodecHttp2::UpdateEncodingDynamicTable(int32 iRecoverSize)
{
    uint32 uiEvictHeaderNum = 0;
    if (iRecoverSize > 0)
    {
        int32 iEvictHeaderSize = 0;
        // determine how many headers need to be evicted.
        for (int32 i = (int32)m_vecEncodingDynamicTable.size() - 1;
                iEvictHeaderSize < iRecoverSize; --i)
        {
            if (i < 0 || i < m_iNextEncodingDynamicTableHeaderIndex)
            {
                break;
            }
            iEvictHeaderSize += m_vecEncodingDynamicTable[i].HpackSize();
            m_uiSettingsHeaderTableSize -= m_vecEncodingDynamicTable[i].HpackSize();
            --m_uiEncodingDynamicTableHeaderCount;
            ++uiEvictHeaderNum;
        }

        // move headers in dynamic table
        uint32 uiFrom = 0;
        uint32 uiTo = 0;
        for (uint32 j = 0; j < uiEvictHeaderNum; ++j)
        {
            uiTo = m_vecEncodingDynamicTable.size() - 1 - j;
            uiFrom = m_vecEncodingDynamicTable.size() - 1 - j - uiEvictHeaderNum;
            m_vecEncodingDynamicTable[uiTo] = std::move(m_vecEncodingDynamicTable[uiFrom]);
        }
        m_iNextEncodingDynamicTableHeaderIndex += uiEvictHeaderNum;
    }
    return(uiEvictHeaderNum);
}

uint32 CodecHttp2::EncodingDynamicTableIndex2VectorIndex(uint32 uiIndex)
{
    return(m_iNextEncodingDynamicTableHeaderIndex + 1 + uiIndex);
}

void CodecHttp2::UpdateDecodingDynamicTable(int32 iVectorIndex, const std::string& strHeaderName, const std::string& strHeaderValue)
{
    Http2Header oHeader(strHeaderName, strHeaderValue);
    uint32 uiEntrySize = oHeader.HpackSize();

    if (iVectorIndex < 0 || iVectorIndex >= (int32)m_vecDecodingDynamicTable.size())    // new header
    {
        // if the new header is too big, drop all entries.
        if (uiEntrySize > m_uiSettingsHeaderTableSize)
        {
            m_uiDecodingDynamicTableSize = 0;
            m_uiDecodingDynamicTableHeaderCount = 0;
            m_iNextDecodingDynamicTableHeaderIndex = m_vecDecodingDynamicTable.size() - 1;
            return;
        }
        // evict headers to required length.
        int32 uiNeedRecoverSize = (m_uiDecodingDynamicTableSize + uiEntrySize) - m_uiSettingsHeaderTableSize;
        UpdateDecodingDynamicTable(uiNeedRecoverSize);
        if (m_uiDecodingDynamicTableHeaderCount + 1 > m_vecDecodingDynamicTable.size()) // need to grow the dynamic table.
        {
            Http2Header oHeader("", "");
            std::vector<Http2Header> vecDynamicTable;
            vecDynamicTable.assign(m_vecDecodingDynamicTable.size() * 2, oHeader);
            for (uint32 i = 0; i < m_vecDecodingDynamicTable.size(); ++i)
            {
                vecDynamicTable[m_vecDecodingDynamicTable.size() + i] = std::move(m_vecDecodingDynamicTable[i]);
            }
            m_iNextDecodingDynamicTableHeaderIndex = m_vecDecodingDynamicTable.size() - 1;
            m_vecDecodingDynamicTable = std::move(vecDynamicTable);
        }
        int iCurrent = m_iNextDecodingDynamicTableHeaderIndex--;
        m_vecDecodingDynamicTable[iCurrent] = std::move(oHeader);
        ++m_uiDecodingDynamicTableHeaderCount;
        m_uiDecodingDynamicTableSize += uiEntrySize;
    }
    else
    {
        int32 iAddSize = uiEntrySize - m_vecDecodingDynamicTable[iVectorIndex].HpackSize();
        // if the replacement header is too big, drop all entries.
        if (iAddSize > (int32)m_uiSettingsHeaderTableSize)
        {
            m_uiDecodingDynamicTableSize = 0;
            m_uiDecodingDynamicTableHeaderCount = 0;
            m_iNextDecodingDynamicTableHeaderIndex = m_vecDecodingDynamicTable.size() - 1;
            return;
        }
        // evict headers to the required length.
        int32 iNeedRecoverSize = (m_uiDecodingDynamicTableSize + iAddSize) - m_uiSettingsHeaderTableSize;
        uint32 uiEvictedNum = UpdateDecodingDynamicTable(iNeedRecoverSize);
        iVectorIndex += uiEvictedNum;
        m_vecDecodingDynamicTable[iVectorIndex] = std::move(oHeader);
        m_uiDecodingDynamicTableSize += iAddSize;
    }
}

uint32 CodecHttp2::UpdateDecodingDynamicTable(int32 iRecoverSize)
{
    uint32 uiEvictHeaderNum = 0;
    if (iRecoverSize > 0)
    {
        int32 iEvictHeaderSize = 0;
        // determine how many headers need to be evicted.
        for (int32 i = (int32)m_vecDecodingDynamicTable.size() - 1;
                iEvictHeaderSize < iRecoverSize; --i)
        {
            if (i < 0 || i < m_iNextDecodingDynamicTableHeaderIndex)
            {
                break;
            }
            iEvictHeaderSize += m_vecDecodingDynamicTable[i].HpackSize();
            m_uiDecodingDynamicTableSize -= m_vecDecodingDynamicTable[i].HpackSize();
            --m_uiDecodingDynamicTableHeaderCount;
            ++uiEvictHeaderNum;
        }

        // move headers in dynamic table
        uint32 uiFrom = 0;
        uint32 uiTo = 0;
        for (uint32 j = 0; j < uiEvictHeaderNum; ++j)
        {
            uiTo = m_vecDecodingDynamicTable.size() - 1 - j;
            uiFrom = m_vecDecodingDynamicTable.size() - 1 - j - uiEvictHeaderNum;
            m_vecDecodingDynamicTable[uiTo] = std::move(m_vecDecodingDynamicTable[uiFrom]);
        }
        m_iNextDecodingDynamicTableHeaderIndex += uiEvictHeaderNum;
    }
    return(uiEvictHeaderNum);
}

uint32 CodecHttp2::DecodingDynamicTableIndex2VectorIndex(uint32 uiIndex)
{
    return(m_iNextDecodingDynamicTableHeaderIndex + 1 + uiIndex);
}

void CodecHttp2::CloseStream(uint32 uiStreamId)
{
    RstStream(uiStreamId);
}

} /* namespace neb */

