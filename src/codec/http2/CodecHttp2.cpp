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
#include "Http2Stream.hpp"
#include "Http2Frame.hpp"
#include "Http2Header.hpp"

namespace neb
{

CodecHttp2::CodecHttp2(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType)
{
#if __cplusplus >= 201401L
    m_pFrame = std::make_unique<Http2Frame>(pLogger, eCodecType);
#else
    m_pFrame = std::unique_ptr<Http2Frame>(new Http2Frame(pLogger, eCodecType));
#endif
}

CodecHttp2::~CodecHttp2()
{
    ReleaseStreamWeight(m_pStreamWeight);
    m_pStreamWeight = nullptr;
    for (auto iter = m_mapStream.begin(); iter != m_mapStream.end(); ++iter)
    {
        delete iter->second;
        iter->second = nullptr;
    }
    m_mapStream.clear();
}

E_CODEC_STATUS CodecHttp2::Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    m_bChunkNotice = oHttpMsg.chunk_notice();
    for (int i = 0; i < oHttpMsg.adding_without_index_headers_size(); ++i)
    {
        m_setEncodingWithoutIndexHeaders.insert(oHttpMsg.adding_without_index_headers(i));
    }
    for (int i = 0; i < oHttpMsg.adding_never_index_headers_size(); ++i)
    {
        m_setEncodingNeverIndexHeaders.insert(oHttpMsg.adding_never_index_headers(i));
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecHttp2::Decode(CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u", pBuff->ReadableBytes());
    if (pBuff->ReadableBytes() <= H2_FRAME_HEAD_SIZE)
    {
        return(CODEC_STATUS_PAUSE);
    }

    int iReadIdx = pBuff->GetReadIndex();
    pBuff->Read(&m_stFrameHead.uiLength, 3);
    pBuff->Read(&m_stFrameHead.ucType, 1);
    pBuff->Read(&m_stFrameHead.ucFlag, 1);
    pBuff->Read(&m_stFrameHead.uiStreamIdentifier, 4);
    m_stFrameHead.cR = (m_stFrameHead.uiStreamIdentifier & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    m_stFrameHead.uiLength = ntohl(m_stFrameHead.uiLength);
    m_stFrameHead.uiStreamIdentifier = ntohl(m_stFrameHead.uiStreamIdentifier & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    if (m_stFrameHead.uiLength > m_uiSettingsMaxFrameSize)
    {
        SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    if (pBuff->ReadableBytes() < m_stFrameHead.uiLength)
    {
        pBuff->SetReadIndex(iReadIdx);
        return(CODEC_STATUS_PAUSE);
    }

    if (m_uiGoawayLastStreamId > 0 && m_stFrameHead.uiStreamIdentifier > m_uiGoawayLastStreamId)
    {
        SetErrno(H2_ERR_CANCEL);
        pBuff->SetReadIndex(iReadIdx);
        return(CODEC_STATUS_PART_ERR);
    }

    if (m_pCodingStream != nullptr)
    {
        if (m_stFrameHead.uiStreamIdentifier == m_pCodingStream->GetStreamId())
        {
            return(m_pCodingStream->Decode(this, m_stFrameHead, pBuff, oHttpMsg, pReactBuff));
        }
        if (!m_pCodingStream->IsEndHeaders())
        {
            // If the END_HEADERS bit is not set, this frame MUST be followed by another CONTINUATION frame.
            // A receiver MUST treat the receipt of any other type of frame or a frame on a different
            // stream as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
            SetErrno(H2_ERR_PROTOCOL_ERROR);
            m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                    "detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
    }

    if (m_stFrameHead.uiStreamIdentifier > 0)
    {
        auto stream_iter = m_mapStream.find(m_stFrameHead.uiStreamIdentifier);
        if (stream_iter == m_mapStream.end())
        {
            /** The identifier of a newly established stream MUST be numerically
             *  greater than all streams that the initiating endpoint has opened
             *  or reserved. This governs streams that are opened using a HEADERS
             *  frame and streams that are reserved using PUSH_PROMISE. An endpoint
             *  that receives an unexpected stream identifier MUST respond with
             *  a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
             */
            if (m_stFrameHead.uiStreamIdentifier <= m_uiStreamIdGenerate)
            {
                SetErrno(H2_ERR_PROTOCOL_ERROR);
                m_pFrame->EncodeGoaway(this, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                        "detected an unspecific protocol error. This error is for "
                        "use when a more specific error code is not available.", pReactBuff);
                return(CODEC_STATUS_ERR);
            }
            m_uiStreamIdGenerate = m_stFrameHead.uiStreamIdentifier;
            try
            {
                m_pCodingStream = new Http2Stream(m_pLogger, GetCodecType());
                m_mapStream.insert(std::make_pair(m_stFrameHead.uiStreamIdentifier, m_pCodingStream));
            }
            catch(std::bad_alloc& e)
            {
                LOG4_ERROR("%s", e.what());
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            m_pCodingStream = stream_iter->second;
        }
        return(m_pCodingStream->Decode(this, m_stFrameHead, pBuff, oHttpMsg, pReactBuff));
    }
    else
    {
        return(m_pFrame->Decode(this, m_stFrameHead, pBuff, oHttpMsg, pReactBuff));
    }
}

void CodecHttp2::SetPriority(uint32 uiStreamId, const tagPriority& stPriority)
{
    if (m_pStreamWeight == nullptr)
    {
        try
        {
            m_pStreamWeight = new TreeNode<tagStreamWeight>();
            m_pStreamWeight->pData = new tagStreamWeight();
            m_pStreamWeight->pData->uiStreamId = stPriority.uiDependency;
            m_pStreamWeight->pFirstChild = new TreeNode<tagStreamWeight>();
            m_pStreamWeight->pFirstChild->pData = new tagStreamWeight();
            m_pStreamWeight->pFirstChild->pData->uiStreamId = uiStreamId;
            m_pStreamWeight->pFirstChild->pData->ucWeight = stPriority.ucWeight;
            m_pStreamWeight->pFirstChild->pParent = m_pStreamWeight;
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("%s", e.what());
            return;
        }
    }

    auto pCurrentStreamWeight = FindStreamWeight(uiStreamId, m_pStreamWeight);
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
    auto pDependencyStreamWeight = FindStreamWeight(stPriority.uiDependency, m_pStreamWeight);
    if (pDependencyStreamWeight == nullptr)
    {
        pCurrentStreamWeight->pRightBrother = m_pStreamWeight->pRightBrother;
        m_pStreamWeight->pRightBrother = pCurrentStreamWeight;
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
        auto pStreamWeight = FindStreamWeight(uiStreamId, m_pStreamWeight);
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

E_H2_ERR_CODE CodecHttp2::Setting(const std::vector<tagSetting>& vecSetting)
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
                    m_uiSettingsMaxFrameSize = vecSetting[i].uiValue;
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
    // TODO window update
}

E_CODEC_STATUS CodecHttp2::UnpackHeader(uint32 uiHeaderBlockEndPos, CBuffer* pBuff, HttpMsg& oHttpMsg)
{
    char B = 0;
    size_t uiReadIndex = 0;

    E_CODEC_STATUS eStatus;
    bool bWithHuffman = false;
    int iDynamicTableIndex = -1;
    std::string strHeaderName;
    std::string strHeaderValue;
    auto pHeader = oHttpMsg.mutable_headers();
    while (pBuff->GetReadIndex() < uiHeaderBlockEndPos)
    {
        uiReadIndex = pBuff->GetReadIndex();
        pBuff->ReadByte(B);
        pBuff->SetReadIndex(uiReadIndex);
        if (H2_HPACK_CONDITION_INDEXED_HEADER & B)
        {
            eStatus = UnpackHeaderIndexed(pBuff, pHeader);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
        }
        else if (H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING & B)
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_6_BITS, iDynamicTableIndex,
                    strHeaderName, strHeaderValue, bWithHuffman);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
            pHeader->insert({strHeaderName, strHeaderValue});
            //UpdateDecodingDynamicTable(iDynamicTableIndex, strHeaderName, strHeaderValue);
            UpdateDecodingDynamicTable(-1, strHeaderName, strHeaderValue);
        }
        else if (H2_HPACK_CONDITION_LITERAL_HEADER_NEVER_INDEXED & B)
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_4_BITS, iDynamicTableIndex,
                    strHeaderName, strHeaderValue, bWithHuffman);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
            pHeader->insert({strHeaderName, strHeaderValue});
            oHttpMsg.add_adding_never_index_headers(strHeaderName);
        }
        else if (H2_HPACK_CONDITION_DYNAMIC_TABLE_SIZE_UPDATE & B)
        {
            uint32 uiTableSize = (uint32)Http2Header::DecodeInt(H2_HPACK_PREFIX_5_BITS, pBuff);
            if (uiTableSize > m_uiSettingsHeaderTableSize)
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("The new maximum size MUST be lower than or equal to "
                        "the limit(SETTINGS_HEADER_TABLE_SIZE) determined by the"
                        " protocol using HPACK!");
                return(CODEC_STATUS_ERR);
            }
            pHeader->insert({strHeaderName, strHeaderValue});
            UpdateDecodingDynamicTable(uiTableSize);
        }
        else    // H2_HPACK_CONDITION_LITERAL_HEADER_WITHOUT_INDEXING
        {
            eStatus = UnpackHeaderLiteralIndexing(pBuff, B,
                    H2_HPACK_PREFIX_4_BITS, iDynamicTableIndex,
                    strHeaderName, strHeaderValue, bWithHuffman);
            if (eStatus != CODEC_STATUS_PART_OK)
            {
                return(eStatus);
            }
            pHeader->insert({strHeaderName, strHeaderValue});
            oHttpMsg.add_adding_without_index_headers(strHeaderName);
        }
    }
    oHttpMsg.set_with_huffman(bWithHuffman);
    return(CODEC_STATUS_PART_OK);
}

void CodecHttp2::PackHeader(const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    size_t uiTableIndex = 0;

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

    for (auto c_iter = oHttpMsg.headers().begin();
            c_iter != oHttpMsg.headers().end(); ++c_iter)
    {
        uiTableIndex = Http2Header::GetStaticTableIndex(c_iter->first, c_iter->second);
        if (uiTableIndex == 0)
        {
            uiTableIndex = GetEncodingTableIndex(c_iter->first, c_iter->second);
        }
        if (uiTableIndex > 0)
        {
            PackHeaderIndexed(uiTableIndex, pBuff);
        }
        else
        {
            auto never_index_iter = m_setEncodingNeverIndexHeaders.find(c_iter->first);
            if (never_index_iter != m_setEncodingNeverIndexHeaders.end())
            {
                PackHeaderNeverIndexing(c_iter->first, c_iter->second, oHttpMsg.with_huffman(), pBuff);
                continue;
            }
            auto without_index_iter = m_setEncodingWithoutIndexHeaders.find(c_iter->first);
            if (without_index_iter != m_setEncodingWithoutIndexHeaders.end())
            {
                PackHeaderWithoutIndexing(c_iter->first, c_iter->second, oHttpMsg.with_huffman(), pBuff);
                continue;
            }
            PackHeaderWithIndexing(c_iter->first, c_iter->second, oHttpMsg.with_huffman(), pBuff);
        }
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
        pPromiseStream = new Http2Stream(m_pLogger, GetCodecType());
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

uint32 CodecHttp2::StreamIdGenerate()
{
    if (m_bChannelIsClient)
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

E_CODEC_STATUS CodecHttp2::UnpackHeaderIndexed(CBuffer* pBuff,
        ::google::protobuf::Map< ::std::string, ::std::string >* pHeader)
{
    uint32 uiTableIndex = (uint32)Http2Header::DecodeInt(H2_HPACK_PREFIX_7_BITS, pBuff);
    if (uiTableIndex == 0)
    {
        SetErrno(H2_ERR_COMPRESSION_ERROR);
        LOG4_ERROR("hpack index value of 0 is not used!");
        return(CODEC_STATUS_ERR);
    }
    else if (uiTableIndex <= Http2Header::sc_uiMaxStaticTableIndex)
    {
        pHeader->insert({
            Http2Header::sc_vecStaticTable[uiTableIndex].first,
            Http2Header::sc_vecStaticTable[uiTableIndex].second});
        return(CODEC_STATUS_PART_OK);
    }
    else if (uiTableIndex <= Http2Header::sc_uiMaxStaticTableIndex + m_vecDecodingDynamicTable.size())
    {
        uint32 uiDynamicTableIndex = uiTableIndex - Http2Header::sc_uiMaxStaticTableIndex - 1;
        pHeader->insert({
            m_vecDecodingDynamicTable[uiDynamicTableIndex].Name(),
            m_vecDecodingDynamicTable[uiDynamicTableIndex].Value()});
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

E_CODEC_STATUS CodecHttp2::UnpackHeaderLiteralIndexing(CBuffer* pBuff, uint8 ucFirstByte, int32 iPrefixMask,
        int& iDynamicTableIndex, std::string& strHeaderName, std::string& strHeaderValue, bool& bWithHuffman)
{
    // Literal Header Field with Incremental Indexing — Indexed Name
    if (iPrefixMask & ucFirstByte)
    {
        uint32 uiTableIndex = (uint32)Http2Header::DecodeInt(iPrefixMask, pBuff);
        if (uiTableIndex == 0)
        {
            SetErrno(H2_ERR_COMPRESSION_ERROR);
            LOG4_ERROR("hpack index value of 0 is not used!");
            return(CODEC_STATUS_ERR);
        }
        else if (uiTableIndex <= Http2Header::sc_uiMaxStaticTableIndex)
        {
            if (!Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("DecodeStringLiteral failed!");
                return(CODEC_STATUS_ERR);
            }
            strHeaderName = Http2Header::sc_vecStaticTable[uiTableIndex].first;
            return(CODEC_STATUS_PART_OK);
        }
        else if (uiTableIndex < Http2Header::sc_uiMaxStaticTableIndex + m_vecDecodingDynamicTable.size())
        {
            uint32 uiDynamicTableIndex = uiTableIndex - Http2Header::sc_uiMaxStaticTableIndex - 1;
            if (!Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
            {
                SetErrno(H2_ERR_COMPRESSION_ERROR);
                LOG4_ERROR("DecodeStringLiteral failed!");
                return(CODEC_STATUS_ERR);
            }
            strHeaderName = m_vecDecodingDynamicTable[uiDynamicTableIndex].Name();
            return(CODEC_STATUS_PART_OK);
        }
        else
        {
            SetErrno(H2_ERR_COMPRESSION_ERROR);
            LOG4_ERROR("hpack index value of %d was greater than the sum of "
                    "the lengths of both static table and dynamic tables!", uiTableIndex);
            return(CODEC_STATUS_ERR);
        }
    }
    else  // Literal Header Field with Incremental Indexing — New Name
    {
        pBuff->SkipBytes(1);
        if (Http2Header::DecodeStringLiteral(pBuff, strHeaderName, bWithHuffman)
            && Http2Header::DecodeStringLiteral(pBuff, strHeaderValue, bWithHuffman))
        {
            return(CODEC_STATUS_PART_OK);
        }
        SetErrno(H2_ERR_COMPRESSION_ERROR);
        LOG4_ERROR("DecodeStringLiteral header name or header value failed!");
        return(CODEC_STATUS_ERR);
    }
}

void CodecHttp2::PackHeaderIndexed(size_t uiTableIndex, CBuffer* pBuff)
{
    Http2Header::EncodeInt(uiTableIndex, (size_t)H2_HPACK_PREFIX_7_BITS,
            (char)H2_HPACK_CONDITION_INDEXED_HEADER, pBuff);
}

void CodecHttp2::PackHeaderWithIndexing(const std::string& strHeaderName,
        const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff)
{
    size_t uiTableIndex = 0;
    uiTableIndex = Http2Header::GetStaticTableIndex(strHeaderName);
    if (uiTableIndex == 0)
    {
        uiTableIndex = GetEncodingTableIndex(strHeaderName);
    }

    if (uiTableIndex > 0)
    {
        Http2Header::EncodeInt(uiTableIndex, (size_t)H2_HPACK_PREFIX_6_BITS,
                (char)H2_HPACK_CONDITION_LITERAL_HEADER_WITH_INDEXING, pBuff);
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
    }
}

void CodecHttp2::PackHeaderWithoutIndexing(const std::string& strHeaderName,
        const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff)
{
    size_t uiTableIndex = 0;
    uiTableIndex = Http2Header::GetStaticTableIndex(strHeaderName);
    if (uiTableIndex == 0)
    {
        uiTableIndex = GetEncodingTableIndex(strHeaderName);
    }

    if (uiTableIndex > 0)
    {
        Http2Header::EncodeInt(uiTableIndex, (size_t)H2_HPACK_PREFIX_4_BITS,
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
        const std::string& strHeaderValue, bool bWithHuffman, CBuffer* pBuff)
{
    size_t uiTableIndex = 0;
    uiTableIndex = Http2Header::GetStaticTableIndex(strHeaderName);
    if (uiTableIndex == 0)
    {
        uiTableIndex = GetEncodingTableIndex(strHeaderName);
    }

    if (uiTableIndex > 0)
    {
        Http2Header::EncodeInt(uiTableIndex, (size_t)H2_HPACK_PREFIX_4_BITS,
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

size_t CodecHttp2::GetEncodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue)
{
    size_t uiTableIndex = 0;
    if (strHeaderValue.size() == 0)
    {
        for (size_t i = 0; i < m_vecEncodingDynamicTable.size(); ++i)
        {
            if (m_vecEncodingDynamicTable[i].Name() == strHeaderName)
            {
                uiTableIndex = Http2Header::sc_uiMaxStaticTableIndex + i + 1;
                break;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < m_vecEncodingDynamicTable.size(); ++i)
        {
            if (m_vecEncodingDynamicTable[i].Name() == strHeaderName
                    && m_vecEncodingDynamicTable[i].Value() == strHeaderValue)
            {
                uiTableIndex = Http2Header::sc_uiMaxStaticTableIndex + i + 1;
                break;
            }
        }
    }
    return(uiTableIndex);
}

//size_t CodecHttp2::GetDecodingTableIndex(const std::string& strHeaderName, const std::string& strHeaderValue)
//{
//    size_t uiTableIndex = 0;
//    if (strHeaderValue.size() == 0)
//    {
//        for (size_t i = 0; i < m_vecDecodingDynamicTable.size(); ++i)
//        {
//            if (m_vecDecodingDynamicTable[i].Name() == strHeaderName)
//            {
//                uiTableIndex = Http2Header::sc_uiMaxStaticTableIndex + i + 1;
//                break;
//            }
//        }
//    }
//    else
//    {
//        for (size_t i = 0; i < m_vecDecodingDynamicTable.size(); ++i)
//        {
//            if (m_vecDecodingDynamicTable[i].Name() == strHeaderName
//                    && m_vecDecodingDynamicTable[i].Value() == strHeaderValue)
//            {
//                uiTableIndex = Http2Header::sc_uiMaxStaticTableIndex + i + 1;
//                break;
//            }
//        }
//    }
//    return(uiTableIndex);
//}

void CodecHttp2::UpdateEncodingDynamicTable(int iDynamicTableIndex, const std::string& strHeaderName, const std::string& strHeaderValue)
{
    Http2Header oHeader(strHeaderName, strHeaderValue);
    if (iDynamicTableIndex < 0 || iDynamicTableIndex >= (int)m_vecEncodingDynamicTable.size())   // new header
    {
        while (m_uiEncodingDynamicTableSize + oHeader.HpackSize() > m_uiSettingsMaxFrameSize
                && m_uiEncodingDynamicTableSize > 0)
        {
            auto& rLastElement = m_vecEncodingDynamicTable.back();
            m_uiEncodingDynamicTableSize -= rLastElement.HpackSize();
            m_vecEncodingDynamicTable.pop_back();
        }
        if (oHeader.HpackSize() <= m_uiSettingsMaxFrameSize)
        {
            m_vecEncodingDynamicTable.insert(m_vecEncodingDynamicTable.begin(),
                    std::move(oHeader));
            m_uiEncodingDynamicTableSize += oHeader.HpackSize();
        }
    }
    else    // replace header ?
    {
        while ((m_uiEncodingDynamicTableSize + oHeader.HpackSize()
                - m_vecEncodingDynamicTable[iDynamicTableIndex].HpackSize())
                > m_uiSettingsMaxFrameSize
                && m_uiEncodingDynamicTableSize > 0)
        {
            auto& rLastElement = m_vecEncodingDynamicTable.back();
            m_uiEncodingDynamicTableSize -= rLastElement.HpackSize();
            m_vecEncodingDynamicTable.pop_back();
        }
        if (oHeader.HpackSize() <= m_uiSettingsMaxFrameSize)
        {
            if (iDynamicTableIndex < (int)m_vecEncodingDynamicTable.size())  // replace header
            {
                m_vecEncodingDynamicTable[iDynamicTableIndex] = std::move(oHeader);
            }
            else    // new header
            {
                m_vecEncodingDynamicTable.insert(m_vecEncodingDynamicTable.begin(),
                        std::move(oHeader));
            }
            m_uiEncodingDynamicTableSize += oHeader.HpackSize();
        }
    }
}

void CodecHttp2::UpdateEncodingDynamicTable(uint32 uiTableSize)
{
    m_uiSettingsMaxFrameSize = uiTableSize;
    while (m_uiEncodingDynamicTableSize > m_uiSettingsMaxFrameSize
            && m_uiEncodingDynamicTableSize > 0)
    {
        auto& rLastElement = m_vecEncodingDynamicTable.back();
        m_uiEncodingDynamicTableSize -= rLastElement.HpackSize();
        m_vecEncodingDynamicTable.pop_back();
    }
}

void CodecHttp2::UpdateDecodingDynamicTable(int iDynamicTableIndex, const std::string& strHeaderName, const std::string& strHeaderValue)
{
    Http2Header oHeader(strHeaderName, strHeaderValue);
    if (iDynamicTableIndex < 0 || iDynamicTableIndex >= (int)m_vecDecodingDynamicTable.size())   // new header
    {
        while (m_uiDecodingDynamicTableSize + oHeader.HpackSize() > m_uiSettingsMaxFrameSize
                && m_uiDecodingDynamicTableSize > 0)
        {
            auto& rLastElement = m_vecDecodingDynamicTable.back();
            m_uiDecodingDynamicTableSize -= rLastElement.HpackSize();
            m_vecDecodingDynamicTable.pop_back();
        }
        if (oHeader.HpackSize() <= m_uiSettingsMaxFrameSize)
        {
            m_vecDecodingDynamicTable.insert(m_vecDecodingDynamicTable.begin(),
                    std::move(oHeader));
            m_uiDecodingDynamicTableSize += oHeader.HpackSize();
        }
    }
    else    // replace header ?
    {
        while ((m_uiDecodingDynamicTableSize + oHeader.HpackSize()
                - m_vecDecodingDynamicTable[iDynamicTableIndex].HpackSize())
                > m_uiSettingsMaxFrameSize
                && m_uiDecodingDynamicTableSize > 0)
        {
            auto& rLastElement = m_vecDecodingDynamicTable.back();
            m_uiDecodingDynamicTableSize -= rLastElement.HpackSize();
            m_vecDecodingDynamicTable.pop_back();
        }
        if (oHeader.HpackSize() <= m_uiSettingsMaxFrameSize)
        {
            if (iDynamicTableIndex < (int)m_vecDecodingDynamicTable.size())  // replace header
            {
                m_vecDecodingDynamicTable[iDynamicTableIndex] = std::move(oHeader);
            }
            else    // new header
            {
                m_vecDecodingDynamicTable.insert(m_vecDecodingDynamicTable.begin(),
                        std::move(oHeader));
            }
            m_uiDecodingDynamicTableSize += oHeader.HpackSize();
        }
    }
}

void CodecHttp2::UpdateDecodingDynamicTable(uint32 uiTableSize)
{
    m_uiSettingsMaxFrameSize = uiTableSize;
    while (m_uiDecodingDynamicTableSize > m_uiSettingsMaxFrameSize
            && m_uiDecodingDynamicTableSize > 0)
    {
        auto& rLastElement = m_vecDecodingDynamicTable.back();
        m_uiDecodingDynamicTableSize -= rLastElement.HpackSize();
        m_vecDecodingDynamicTable.pop_back();
    }
}

} /* namespace neb */

