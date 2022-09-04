/*******************************************************************************
 * Project:  Nebula
 * @file     CodecResp.cpp
 * @brief
 * @author   nebim
 * @date:    2020-10-02
 * @note
 * Modify history:
 ******************************************************************************/
#include "CodecResp.hpp"
#include "util/StringConverter.hpp"

namespace neb
{

const char CodecResp::RESP_SIMPLE_STRING = '+';
const char CodecResp::RESP_ERROR = '-';
const char CodecResp::RESP_INTEGER = ':';
const char CodecResp::RESP_BULK_STRING = '$';
const char CodecResp::RESP_ARRAY = '*';

CodecResp::CodecResp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
        std::shared_ptr<SocketChannel> pBindChannel)
    : Codec(pLogger, eCodecType, pBindChannel)
{
}

CodecResp::~CodecResp()
{
}

E_CODEC_STATUS CodecResp::Encode(CBuffer* pBuff)
{
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::Encode(const RedisReply& oReply, CBuffer* pBuff)
{
    int iHadWriteLen = 0;
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    switch (oReply.type())
    {
        case REDIS_REPLY_STRING:
            eStatus = EncodeBulkString(oReply, pBuff);
            break;
        case REDIS_REPLY_ARRAY:
            eStatus = EncodeArray(oReply, pBuff);
            break;
        case REDIS_REPLY_STATUS:
            eStatus = EncodeSimpleString(oReply, pBuff);
            break;
        case REDIS_REPLY_INTEGER:
            eStatus = EncodeInteger(oReply, pBuff);
            break;
        case REDIS_REPLY_ERROR:
            eStatus = EncodeError(oReply, pBuff);
            break;
        case REDIS_REPLY_NIL:
            eStatus = EncodeNull(oReply, pBuff);
            break;
        default:
            LOG4_ERROR("invalid redis reply type %d", oReply.type());
            return(CODEC_STATUS_ERR);
    }
    if (CODEC_STATUS_OK != eStatus)
    {
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
    }
    return(eStatus);
}

E_CODEC_STATUS CodecResp::Decode(CBuffer* pBuff, RedisReply& oReply)
{
    if (pBuff->ReadableBytes() < 1)
    {
        ++m_uiIncompleteDecodeNum;
        return(CODEC_STATUS_PAUSE);
    }
    if (m_bDecodeWithStack)
    {
        return(DecodeWithStack(pBuff, oReply));
    }
    else
    {
        if (m_uiIncompleteDecodeNum > 100
                && (m_uiDecodedNum / m_uiIncompleteDecodeNum < 5)) // Over 20% of decoding is incomple
        {
            m_bDecodeWithStack = true;
            return(Decode(pBuff, oReply));
        }
    }
    return(DecodeRecursive(pBuff, oReply));
}

E_CODEC_STATUS CodecResp::EncodeSimpleString(const RedisReply& oReply, CBuffer* pBuff)
{
    pBuff->WriteByte(RESP_SIMPLE_STRING);
    pBuff->Write(oReply.str().c_str(), oReply.str().size());
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::EncodeError(const RedisReply& oReply, CBuffer* pBuff)
{
    pBuff->WriteByte(RESP_ERROR);
    pBuff->Write(oReply.str().c_str(), oReply.str().size());
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::EncodeInteger(const RedisReply& oReply, CBuffer* pBuff)
{
    char szInteger[64] = {0};
    snprintf(szInteger, 64, "%ld", oReply.integer());
    pBuff->WriteByte(RESP_INTEGER);
    pBuff->Write(&szInteger, strlen(szInteger));
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::EncodeBulkString(const RedisReply& oReply, CBuffer* pBuff)
{
    char szStringLen[64] = {0};
    snprintf(szStringLen, 64, "%lu", oReply.str().size());
    pBuff->WriteByte(RESP_BULK_STRING);
    pBuff->Write(szStringLen, strlen(szStringLen));
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    if (oReply.str().size() > 0)
    {
        pBuff->Write(oReply.str().c_str(), oReply.str().size());
    }
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::EncodeArray(const RedisReply& oReply, CBuffer* pBuff)
{
    char szArraySize[64] = {0};
    snprintf(szArraySize, 64, "%d", oReply.element_size());
    pBuff->WriteByte(RESP_ARRAY);
    pBuff->Write(szArraySize, strlen(szArraySize));
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    for (int i = 0; i < oReply.element_size(); ++i)
    {
        auto& rElement = oReply.element(i);
        switch (rElement.type())
        {
            case REDIS_REPLY_STRING:
                eStatus = EncodeBulkString(rElement, pBuff);
                break;
            case REDIS_REPLY_ARRAY:
                eStatus = EncodeArray(rElement, pBuff);
                break;
            case REDIS_REPLY_INTEGER:
                eStatus = EncodeInteger(rElement, pBuff);
                break;
            case REDIS_REPLY_STATUS:
                eStatus = EncodeSimpleString(rElement, pBuff);
                break;
            case REDIS_REPLY_ERROR:
                eStatus = EncodeError(rElement, pBuff);
                break;
            case REDIS_REPLY_NIL:
                eStatus = EncodeNull(rElement, pBuff);
                break;
            default:
                LOG4_ERROR("invalid redis reply type %d", oReply.type());
                return(CODEC_STATUS_ERR);
        }
        if (CODEC_STATUS_OK != eStatus)
        {
            return(eStatus);
        }
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::EncodeNull(const RedisReply& oReply, CBuffer* pBuff)
{
    char szInteger[64] = {0};
    snprintf(szInteger, 64, "%d", -1);
    pBuff->WriteByte(RESP_BULK_STRING);
    pBuff->Write(&szInteger, strlen(szInteger));
    pBuff->WriteByte('\r');
    pBuff->WriteByte('\n');
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecResp::DecodeRecursive(CBuffer* pBuff, RedisReply& oReply)
{
    char cFirstByte = 0;
    size_t uiReadIndex = pBuff->GetReadIndex();
    pBuff->ReadByte(cFirstByte);
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    switch (cFirstByte)
    {
        case RESP_SIMPLE_STRING:
            eStatus = DecodeSimpleString(pBuff, oReply);
            break;
        case RESP_INTEGER:
            eStatus = DecodeInteger(pBuff, oReply);
            break;
        case RESP_BULK_STRING:
            eStatus = DecodeBulkString(pBuff, oReply);
            break;
        case RESP_ARRAY:
            eStatus = DecodeArray(pBuff, oReply);
            break;
        case RESP_ERROR:
            eStatus = DecodeError(pBuff, oReply);
            break;
        default:
            oReply.set_type(REDIS_REPLY_ERROR);
            oReply.set_integer(REDIS_ERR_PROTOCOL);
            pBuff->SetReadIndex(uiReadIndex);
            LOG4_TRACE("cFirstByte = %d", int(cFirstByte));
            return(CODEC_STATUS_ERR);
    }
    if (CODEC_STATUS_OK != eStatus)
    {
        pBuff->SetReadIndex(uiReadIndex);
    }
    return(eStatus);
}

E_CODEC_STATUS CodecResp::DecodeSimpleString(CBuffer* pBuff, RedisReply& oReply)
{
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    uint32 uiStringLen = 0;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    oReply.set_type(REDIS_REPLY_STATUS);
                    oReply.set_str(pPartBegin, uiStringLen);
                    pBuff->AdvanceReadIndex(i + 1);
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, (int)cLastChar, (int)pData[i]);
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ++uiStringLen;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeError(CBuffer* pBuff, RedisReply& oReply)
{
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    uint32 uiStringLen = 0;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    oReply.set_type(REDIS_REPLY_ERROR);
                    oReply.set_str(pPartBegin, uiStringLen);
                    pBuff->AdvanceReadIndex(i + 1);
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, (int)cLastChar, (int)pData[i]);
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ++uiStringLen;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeInteger(CBuffer* pBuff, RedisReply& oReply)
{
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    oReply.set_type(REDIS_REPLY_INTEGER);
                    oReply.set_integer(StringConverter::RapidAtoi<int64>(pPartBegin));
                    pBuff->AdvanceReadIndex(i + 1);
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, (int)cLastChar, (int)pData[i]);
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeBulkString(CBuffer* pBuff, RedisReply& oReply)
{
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    int32 iStringLen = 0;
    bool bGotStringLen = false;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    if (bGotStringLen)
                    {
                        oReply.set_type(REDIS_REPLY_STRING);
                        oReply.set_str(pPartBegin, iStringLen);
                        pBuff->AdvanceReadIndex(i + 1);
                        return(CODEC_STATUS_OK);
                    }
                    else
                    {
                        iStringLen = StringConverter::RapidAtoi<int32>(pPartBegin);
                        pPartBegin = pData + i + 1;
                        bGotStringLen = true;
                        if (iStringLen == -1)
                        {
                            oReply.set_type(REDIS_REPLY_NIL);
                            pBuff->AdvanceReadIndex(i + 1);
                            return(CODEC_STATUS_OK);
                        }
                        i += (uint32)iStringLen;
                        if (uiReadableBytes < (i + 1 + 2))   // not enough data (pos i+1 is the end of bulk string)
                        {
                            return(CODEC_STATUS_PAUSE);
                        }
                    }
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, iStringLen = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, iStringLen, (int)cLastChar, (int)pData[i]);
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeArray(CBuffer* pBuff, RedisReply& oReply)
{
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    int32 iArraySize = 0;
    size_t uiReadIndex = pBuff->GetReadIndex();
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    iArraySize = StringConverter::RapidAtoi<int32>(pPartBegin);
                    pBuff->AdvanceReadIndex(i + 1);

                    if (iArraySize == -1)
                    {
                        oReply.set_type(REDIS_REPLY_NIL);
                        return(CODEC_STATUS_OK);
                    }
                    else
                    {
                        oReply.set_type(REDIS_REPLY_ARRAY);
                        for (int32 j = 0; j < iArraySize; ++j)
                        {
                            if (pBuff->ReadableBytes() == 0)
                            {
                                return(CODEC_STATUS_PAUSE);
                            }
                            uiReadIndex = pBuff->GetReadIndex();
                            char cFirstByte = 0;
                            auto pElement = oReply.add_element();
                            pBuff->ReadByte(cFirstByte);
                            E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
                            switch (cFirstByte)
                            {
                                case RESP_SIMPLE_STRING:
                                    eStatus = DecodeSimpleString(pBuff, *pElement);
                                    break;
                                case RESP_INTEGER:
                                    eStatus = DecodeInteger(pBuff, *pElement);
                                    break;
                                case RESP_BULK_STRING:
                                    eStatus = DecodeBulkString(pBuff, *pElement);
                                    break;
                                case RESP_ARRAY:
                                    eStatus = DecodeArray(pBuff, *pElement);
                                    break;
                                case RESP_ERROR:
                                    eStatus = DecodeError(pBuff, *pElement);
                                    break;
                                default:
                                    oReply.set_type(REDIS_REPLY_ERROR);
                                    oReply.set_integer(REDIS_ERR_PROTOCOL);
                                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cFirstByte = %d, cLastChar = %d, pData[i] = %d",
                                            uiReadableBytes, i, int(cFirstByte), (int)cLastChar, (int)pData[i]);
                                    return(CODEC_STATUS_ERR);
                            }
                            if (CODEC_STATUS_OK != eStatus)
                            {
                                return(eStatus);
                            }
                            i += (pBuff->GetReadIndex() - uiReadIndex);
                        }
                        return(CODEC_STATUS_OK);
                    }
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, (int)cLastChar, (int)pData[i]);
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeWithStack(CBuffer* pBuff, RedisReply& oReply)
{
    if (m_iCurrentReadTaskIndex == -1)
    {
        m_oReadStack[0].Reset();
        m_iCurrentReadTaskIndex = 0;
    }

    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    /* Process items in reply. */
    while (m_iCurrentReadTaskIndex >= 0)
    {
        eStatus = DecodeWithStack(pBuff);
        if (eStatus != CODEC_STATUS_OK)
        {
            if (eStatus == CODEC_STATUS_ERR)
            {
                oReply.set_type(REDIS_REPLY_ERROR);
                oReply.set_integer(m_iErrno);
                oReply.set_str(m_szErrMsg);
            }
            return(eStatus);
        }
    }

    /* Emit a reply when there is one. */
    if (m_iCurrentReadTaskIndex == -1)
    {
        ++m_uiDecodedNum;
        oReply = std::move(m_oReply);
    }
    else
    {
        ++m_uiIncompleteDecodeNum;
    }
    return(eStatus);
}

E_CODEC_STATUS CodecResp::DecodeWithStack(CBuffer* pBuff)
{
    CodecRespReadTask* pCurrent = &(m_oReadStack[m_iCurrentReadTaskIndex]);
    if (pBuff->ReadableBytes() < 1)
    {
        return(CODEC_STATUS_PAUSE);
    }

    /* check if we need to read type */
    if (pCurrent->iType < 0)
    {
        char cFirstByte = 0;
        pBuff->ReadByte(cFirstByte);
        switch (cFirstByte)
        {
            case RESP_SIMPLE_STRING:
                pCurrent->iType = REDIS_REPLY_STATUS;
                break;
            case RESP_INTEGER:
                pCurrent->iType = REDIS_REPLY_INTEGER;
                break;
            case RESP_BULK_STRING:
                pCurrent->iType = REDIS_REPLY_STRING;
                break;
            case RESP_ARRAY:
                pCurrent->iType = REDIS_REPLY_ARRAY;
                break;
            case RESP_ERROR:
                pCurrent->iType = REDIS_REPLY_ERROR;
                break;
            default:
                m_iErrno = REDIS_ERR_PROTOCOL;
                snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                    "Protocol error, got %c as reply type byte", cFirstByte);
                m_oReply.Clear();   // TODO 确认一下是否会有内存泄露
                LOG4_ERROR("Protocol error, got %c as reply type byte", cFirstByte);
                return(CODEC_STATUS_ERR);
        }
    }


    /* process typed item */
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    switch(pCurrent->iType)
    {
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_INTEGER:
        case REDIS_REPLY_ERROR:
            eStatus = DecodeLineItem(pBuff);
            break;
        case REDIS_REPLY_STRING:
            eStatus = DecodeBulkItem(pBuff);
            break;
        case REDIS_REPLY_ARRAY:
            eStatus = DecodeArray(pBuff);
            break;
        default:
            m_iErrno = REDIS_ERR_PROTOCOL;
            snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                "Protocol error, reply type %d", pCurrent->iType);
            LOG4_ERROR("Protocol error, reply type %d", pCurrent->iType);
            return(CODEC_STATUS_ERR);
    }
    return(eStatus);
}

E_CODEC_STATUS CodecResp::DecodeLineItem(CBuffer* pBuff)
{
    CodecRespReadTask* pCurrent = &(m_oReadStack[m_iCurrentReadTaskIndex]);
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    uint32 uiStringLen = 0;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    pCurrent->pReply = NewReply(pCurrent->pParent);
                    if (pCurrent->pReply == nullptr)
                    {
                        return(CODEC_STATUS_ERR);
                    }

                    if (REDIS_REPLY_INTEGER == pCurrent->iType)
                    {
                        pCurrent->pReply->set_type(REDIS_REPLY_INTEGER);
                        pCurrent->pReply->set_integer(StringConverter::RapidAtoi<int64>(pPartBegin));
                    }
                    else
                    {
                        // Type will be error or status.
                        pCurrent->pReply->set_type(pCurrent->iType);
                        pCurrent->pReply->set_str(pPartBegin, uiStringLen);
                    }
                    pBuff->AdvanceReadIndex(i + 1);

                    MoveToNextTask();
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, int(cLastChar), (int)pData[i]);
                    m_iErrno = REDIS_ERR_PROTOCOL;
                    snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                        "Protocol error, reply type %d, last char ascii %d",
                        pCurrent->iType, int(cLastChar));
                    m_oReply.Clear();
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ++uiStringLen;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeBulkItem(CBuffer* pBuff)
{
    CodecRespReadTask* pCurrent = &(m_oReadStack[m_iCurrentReadTaskIndex]);
    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    int32 iStringLen = 0;
    bool bGotStringLen = false;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    if (bGotStringLen)
                    {
                        pCurrent->pReply = NewReply(pCurrent->pParent);
                        if (pCurrent->pReply == nullptr)
                        {
                            return(CODEC_STATUS_ERR);
                        }
                        pCurrent->pReply->set_type(pCurrent->iType);
                        pCurrent->pReply->set_str(pPartBegin, iStringLen);
                        pBuff->AdvanceReadIndex(i + 1);
                        MoveToNextTask();
                        return(CODEC_STATUS_OK);
                    }
                    else
                    {
                        iStringLen = StringConverter::RapidAtoi<int32>(pPartBegin);
                        pPartBegin = pData + i + 1;
                        bGotStringLen = true;
                        if (iStringLen == -1)
                        {
                            pCurrent->pReply = NewReply(pCurrent->pParent);
                            if (pCurrent->pReply == nullptr)
                            {
                                return(CODEC_STATUS_ERR);
                            }
                            pCurrent->pReply->set_type(REDIS_REPLY_NIL);
                            pBuff->AdvanceReadIndex(i + 1);
                            MoveToNextTask();
                            return(CODEC_STATUS_OK);
                        }
                        i += (uint32)iStringLen;
                        if (uiReadableBytes < (i + 1 + 2)) // not enough data (pos i + 1 is the end of bulk string)
                        {
                            return(CODEC_STATUS_PAUSE);
                        }
                    }
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, iStringLen = %d, cLastChar = %d, pData[i] = %d",
                            uiReadableBytes, i, iStringLen, int(cLastChar), (int)pData[i]);
                    m_iErrno = REDIS_ERR_PROTOCOL;
                    snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                        "Protocol error, reply type %d, last char ascii %d",
                        pCurrent->iType, int(cLastChar));
                    m_oReply.Clear();
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

E_CODEC_STATUS CodecResp::DecodeArray(CBuffer* pBuff)
{
    CodecRespReadTask* pCurrent = &(m_oReadStack[m_iCurrentReadTaskIndex]);

    /* Set error for nested multi bulks with depth > 7 */
    if (m_iCurrentReadTaskIndex == 8)
    {
        m_iErrno = REDIS_ERR_PROTOCOL;
        snprintf(m_szErrMsg, sizeof(m_szErrMsg),
            "No support for nested multi bulk replies with depth > 7");
        m_oReply.Clear();
        LOG4_ERROR("No support for nested multi bulk replies with depth > 7");
        return(CODEC_STATUS_ERR);
    }

    size_t uiReadableBytes = pBuff->ReadableBytes();
    char cLastChar = 0;
    const char* pData = pBuff->GetRawReadBuffer();
    const char* pPartBegin = pData;
    for (size_t i = 0; i < uiReadableBytes; ++i)
    {
        switch (pData[i])
        {
            case '\n':
                if ('\r' == cLastChar)
                {
                    cLastChar = '\n';
                    pCurrent->iElements = StringConverter::RapidAtoi<int32>(pPartBegin);
                    pBuff->AdvanceReadIndex(i + 1);
                    pCurrent->pReply = NewReply(pCurrent->pParent);
                    if (pCurrent->pReply == nullptr)
                    {
                        return(CODEC_STATUS_ERR);
                    }

                    if (pCurrent->iElements == -1)
                    {
                        pCurrent->pReply->set_type(REDIS_REPLY_NIL);
                        MoveToNextTask();
                        return(CODEC_STATUS_OK);
                    }
                    else
                    {
                        pCurrent->pReply->set_type(pCurrent->iType);

                        /* Modify task stack when there are more than 0 elements. */
                        if (pCurrent->iElements > 0)
                        {
                            m_iCurrentReadTaskIndex++;
                            m_oReadStack[m_iCurrentReadTaskIndex].iType = -1;
                            m_oReadStack[m_iCurrentReadTaskIndex].iElements = -1;
                            m_oReadStack[m_iCurrentReadTaskIndex].iIndex = 0;
                            m_oReadStack[m_iCurrentReadTaskIndex].pReply = nullptr;
                            m_oReadStack[m_iCurrentReadTaskIndex].pParent = pCurrent;
                        }
                        else
                        {
                            MoveToNextTask();
                        }
                        return(CODEC_STATUS_OK);
                    }
                }
                else
                {
                    LOG4_ERROR("uiReadableBytes = %u, i = %d, cLastChar = %d, pData[i] = %d",
                        uiReadableBytes, i, (int)cLastChar, (int)pData[i]);
                    m_iErrno = REDIS_ERR_PROTOCOL;
                    snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                        "Protocol error, reply type %d, last char ascii %d",
                        pCurrent->iType, int(cLastChar));
                    m_oReply.Clear();
                    return(CODEC_STATUS_ERR);
                }
                break;
            case '\r':
                cLastChar = '\r';
                break;
            default:
                ;
        }
    }
    return(CODEC_STATUS_PAUSE);
}

void CodecResp::MoveToNextTask()
{
    CodecRespReadTask* pCurrent;
    CodecRespReadTask* pPrevious;
    while (m_iCurrentReadTaskIndex >= 0)
    {
        /* Return a.s.a.p. when the stack is now empty. */
        if (m_iCurrentReadTaskIndex == 0)
        {
            --m_iCurrentReadTaskIndex;
            return;
        }

        pCurrent = &(m_oReadStack[m_iCurrentReadTaskIndex]);
        pPrevious = &(m_oReadStack[m_iCurrentReadTaskIndex - 1]);
        if (pCurrent->iIndex == pPrevious->iElements - 1)
        {
            --m_iCurrentReadTaskIndex;
        }
        else
        {
            /* Reset the type because the next item can be anything */
            pCurrent->iType = -1;
            pCurrent->iElements = -1;
            pCurrent->iIndex++;
            return;
        }
    }
}

RedisReply* CodecResp::NewReply(CodecRespReadTask *pParent)
{
    RedisReply* pReply = nullptr;
    if (pParent)
    {
        auto pParentReply = pParent->pReply;
        if (pParentReply == nullptr)
        {
            LOG4_ERROR("parent reply is nullptr.");
            m_iErrno = REDIS_ERR;
            snprintf(m_szErrMsg, sizeof(m_szErrMsg), "parent reply is nullptr.");
            m_oReply.Clear();
            return(pReply);
        }
        pReply = pParentReply->add_element();
        if (pReply == nullptr)
        {
            LOG4_ERROR("Out of memory");
            m_iErrno = REDIS_ERR;
            snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Out of memory");
            m_oReply.Clear();
        }
        return(pReply);
    }
    else
    {
        // Set reply if this is the root object.
        if (m_iCurrentReadTaskIndex == 0)
        {
            return(&m_oReply);
        }
        LOG4_ERROR("reply is not root and parent reply is nullptr.");
        m_iErrno = REDIS_ERR;
        snprintf(m_szErrMsg, sizeof(m_szErrMsg), "reply is not root and parent reply is nullptr.");
        m_oReply.Clear();
        return(pReply);
    }
}

E_CODEC_STATUS CodecResp::Decode(CBuffer* pBuff, RedisReply& oReply, CBuffer* pReactBuff)
{
    LOG4_ERROR("invalid");
    return(CODEC_STATUS_ERR);
}

}

