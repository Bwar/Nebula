/*******************************************************************************
 * Project:  Nebula
 * @file     CodecWsExtentJson.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年9月3日
 * @note
 * Modify history:
 ******************************************************************************/
#include <google/protobuf/util/json_util.h>
#include "logger/NetLogger.hpp"
#include "CodecWsExtentJson.hpp"

namespace neb
{

CodecWsExtentJson::CodecWsExtentJson(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType),
      uiBeatCmd(0), uiBeatSeq(0)
{
}

CodecWsExtentJson::~CodecWsExtentJson()
{
}

E_CODEC_STATUS CodecWsExtentJson::Encode(const MsgHead& oMsgHead,
        const MsgBody& oMsgBody, CBuffer* pBuff)
{
    LOG4_TRACE(" ");
    uint8 ucFirstByte = 0;
    uint8 ucSecondByte = 0;
    tagMsgHead stMsgHead;
    stMsgHead.version = 1;        // version暂时无用
    stMsgHead.encript = (unsigned char) (oMsgHead.cmd() >> 24);
    stMsgHead.cmd = htons((unsigned short) (gc_uiCmdBit & oMsgHead.cmd()));
    stMsgHead.body_len = htonl((unsigned int) oMsgHead.len());
    stMsgHead.seq = htonl(oMsgHead.seq());
//stMsgHead.checksum = htons((unsigned short)stMsgHead.checksum);
    if (oMsgBody.ByteSize() > 1000000) // pb 最大限制
    {
        LOG4_ERROR("oMsgBody.ByteSize() > 1000000");
        return (CODEC_STATUS_ERR);
    }
    int iNeedWriteLen = 0;
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    LOG4_TRACE("cmd %u, seq %u, len %u", oMsgHead.cmd(), oMsgHead.seq(), oMsgHead.len());
    if (oMsgHead.len() == 0)    // 无包体（心跳包等）
    {
        ucFirstByte |= WEBSOCKET_FIN;
        if (gc_uiCmdReq & oMsgHead.cmd())   // 发送心跳
        {
            if (uiBeatSeq > 0)
            {
                LOG4_WARNING("sending a new beat while last beat had not been callback.");
                return(CODEC_STATUS_OK);
            }
            uiBeatCmd = oMsgHead.cmd();
            uiBeatSeq = oMsgHead.seq();
            ucFirstByte |= WEBSOCKET_FRAME_PING;
        }
        else
        {
            ucFirstByte |= WEBSOCKET_FRAME_PONG;
        }
        iWriteLen = pBuff->Write(&ucFirstByte, 1);
        iWriteLen = pBuff->Write(&ucSecondByte, 1);
        return (CODEC_STATUS_OK);
    }
    else
    {
        std::string strJsonBody;
        google::protobuf::util::JsonPrintOptions oJsonOption;
        google::protobuf::util::Status oStatus;
        ucFirstByte |= WEBSOCKET_FIN;
        ucFirstByte |= WEBSOCKET_FRAME_BINARY;
        ucSecondByte &= (~WEBSOCKET_PAYLOAD_LEN);
        std::string strCompressData;
        std::string strEncryptData;
        oStatus = google::protobuf::util::MessageToJsonString(oMsgBody, &strJsonBody, oJsonOption);
        if (!oStatus.ok())
        {
            LOG4_ERROR("MsgBody to json string error!");
            return (CODEC_STATUS_ERR);
        }
        if (gc_uiZipBit & oMsgHead.cmd())
        {
            if (!Zip(strJsonBody, strCompressData))
            {
                LOG4_ERROR("zip error!");
                return (CODEC_STATUS_ERR);
            }
        }
        else if (gc_uiGzipBit & oMsgHead.cmd())
        {
            if (!Gzip(strJsonBody, strCompressData))
            {
                LOG4_ERROR("gzip error!");
                return (CODEC_STATUS_ERR);
            }
        }
        if (gc_uiRc5Bit & oMsgHead.cmd())
        {
            if (strCompressData.size() > 0)
            {
                if (!Rc5Encrypt(strCompressData, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return (CODEC_STATUS_ERR);
                }
            }
            else
            {
                if (!Rc5Encrypt(strJsonBody, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return (CODEC_STATUS_ERR);
                }
            }
        }

        if (strEncryptData.size() > 0)              // 加密后的数据包
        {
            stMsgHead.body_len = htonl((unsigned int) strEncryptData.size());
        }
        else if (strCompressData.size() > 0)        // 压缩后的数据包
        {
            stMsgHead.body_len = htonl((unsigned int) strCompressData.size());
        }
        else    // 不需要压缩加密或无效的压缩或加密算法，打包原数据
        {
            iNeedWriteLen = strJsonBody.size();
        }

        iNeedWriteLen = sizeof(stMsgHead) + oMsgHead.len();
        if (iNeedWriteLen > 65535)
        {
            ucSecondByte |= WEBSOCKET_PAYLOAD_LEN_UINT64;
            iWriteLen = pBuff->Write(&ucFirstByte, 1);
            iHadWriteLen += iWriteLen;
            iWriteLen = pBuff->Write(&ucSecondByte, 1);
            iHadWriteLen += iWriteLen;
            uint64 ullPayload = iNeedWriteLen;
            ullPayload = htonll(ullPayload);
            iWriteLen = pBuff->Write(&ullPayload, sizeof(ullPayload));
            iHadWriteLen += iWriteLen;
        }
        else if (iNeedWriteLen >= 126)
        {
            ucSecondByte |= WEBSOCKET_PAYLOAD_LEN_UINT16;
            iWriteLen = pBuff->Write(&ucFirstByte, 1);
            iHadWriteLen += iWriteLen;
            iWriteLen = pBuff->Write(&ucSecondByte, 1);
            iHadWriteLen += iWriteLen;
            uint16 unPayload = iNeedWriteLen;
            unPayload = htons(unPayload);
            iWriteLen = pBuff->Write(&unPayload, sizeof(unPayload));
            iHadWriteLen += iWriteLen;
        }
        else
        {
            ucSecondByte |= iNeedWriteLen;
            iWriteLen = pBuff->Write(&ucFirstByte, 1);
            iHadWriteLen += iWriteLen;
            iWriteLen = pBuff->Write(&ucSecondByte, 1);
            iHadWriteLen += iWriteLen;
        }

        iNeedWriteLen = sizeof(stMsgHead);
        iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d",
                sizeof(stMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return (CODEC_STATUS_ERR);
        }

        if (strEncryptData.size() > 0)              // 加密后的数据包
        {
            iNeedWriteLen = strEncryptData.size();
            iWriteLen = pBuff->Write(strEncryptData.c_str(), strEncryptData.size());
        }
        else if (strCompressData.size() > 0)        // 压缩后的数据包
        {
            iNeedWriteLen = strCompressData.size();
            iWriteLen = pBuff->Write(strCompressData.c_str(), strCompressData.size());
        }
        else    // 无效的压缩或加密算法，打包原数据
        {
            iNeedWriteLen = oMsgBody.ByteSize();
            iWriteLen = pBuff->Write(strJsonBody.c_str(), strJsonBody.size());
        }
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff iWriteLen != iNeedWriteLen");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return (CODEC_STATUS_ERR);
        }
        iHadWriteLen += iWriteLen;
    }
    LOG4_TRACE("oMsgBody.ByteSize() = %d, iWriteLen = %d(compress or encrypt maybe)",
            oMsgBody.ByteSize(), iWriteLen);
    return (CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecWsExtentJson::Decode(CBuffer* pBuff,
        MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u", pBuff->ReadableBytes());
    size_t uiHeadSize = sizeof(tagMsgHead);
    if (pBuff->ReadableBytes() >= 2)
    {
        int iReadIdx = pBuff->GetReadIndex();
        uint8 ucFirstByte = 0;
        uint8 ucSecondByte = 0;
        pBuff->Read(&ucFirstByte, 1);
        pBuff->Read(&ucSecondByte, 1);
        if (!(WEBSOCKET_MASK & ucSecondByte))
        {
            LOG4_ERROR("a masked frame MUST have the field frame-masked set to 1 when client to server!");
            return (CODEC_STATUS_ERR);
        }
        if (0 == (WEBSOCKET_PAYLOAD_LEN & ucSecondByte))    // ping or pong
        {
            if (WEBSOCKET_FRAME_PING & ucFirstByte)
            {
                oMsgHead.set_cmd(uiBeatCmd);
                oMsgHead.set_seq(0);
                oMsgHead.set_len(0);
            }
            else
            {
                oMsgHead.set_cmd(uiBeatCmd + 1);
                oMsgHead.set_seq(uiBeatSeq);
                oMsgHead.set_len(0);
                uiBeatSeq = 0;
            }
            return (CODEC_STATUS_OK);
        }

        char cData;
        const char* pRawData;
        char szMaskKey[4] = {0};
        uint32 uiPayload = 0;
        if (WEBSOCKET_PAYLOAD_LEN_UINT64 == (WEBSOCKET_PAYLOAD_LEN & ucSecondByte))
        {
            uint64 ullPayload = 0;
            if (pBuff->ReadableBytes() <= 12)   // 8 + 4
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            pBuff->Read(&ullPayload, 8);
            pBuff->Read(&szMaskKey, 4);
            if (pBuff->ReadableBytes() < ullPayload)
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            uiPayload = (uint32)ullPayload;
        }
        else if (WEBSOCKET_PAYLOAD_LEN_UINT16 == (WEBSOCKET_PAYLOAD_LEN & ucSecondByte))
        {
            uint16 unPayload = 0;
            if (pBuff->ReadableBytes() <= 6)   // 2 + 4
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            pBuff->Read(&unPayload, 2);
            pBuff->Read(&szMaskKey, 4);
            if (pBuff->ReadableBytes() < unPayload)
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            uiPayload = (uint32)unPayload;
        }
        else
        {
            uint8 ucPayload = 0;
            if (pBuff->ReadableBytes() <= 4)   // 4
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            ucPayload = WEBSOCKET_PAYLOAD_LEN & ucSecondByte;
            pBuff->Read(&szMaskKey, 4);
            if (pBuff->ReadableBytes() < ucPayload)
            {
                pBuff->SetReadIndex(iReadIdx);
                return (CODEC_STATUS_PAUSE);
            }
            uiPayload = (uint32)ucPayload;
        }
        pRawData = pBuff->GetRawReadBuffer();
        for (uint32 i = pBuff->GetReadIndex(), j = 0; j < uiPayload; ++i, ++j)
        {
            cData = pRawData[i] ^ szMaskKey[j % 4];
            pBuff->SetBytes(&cData, 1, i);
        }
        tagMsgHead stMsgHead;
        pBuff->Read(&stMsgHead, uiHeadSize);
        stMsgHead.cmd = ntohs(stMsgHead.cmd);
        stMsgHead.body_len = ntohl(stMsgHead.body_len);
        stMsgHead.seq = ntohl(stMsgHead.seq);
        stMsgHead.checksum = ntohs(stMsgHead.checksum);
        LOG4_TRACE("cmd %u, seq %u, len %u, pBuff->ReadableBytes() %u",
                stMsgHead.cmd, stMsgHead.seq, stMsgHead.body_len,
                pBuff->ReadableBytes());
        oMsgHead.set_cmd(((unsigned int) stMsgHead.encript << 24) | stMsgHead.cmd);
        oMsgHead.set_len(stMsgHead.body_len);
        oMsgHead.set_seq(stMsgHead.seq);
        if (uiHeadSize + stMsgHead.body_len != uiPayload)      // 数据包错误
        {
            LOG4_ERROR("uiHeadSize(%u) + stMsgHead.body_len(%u) != uiPayload(%u)",
                    uiHeadSize, stMsgHead.body_len, uiPayload);
            return (CODEC_STATUS_ERR);
        }

        std::string strJsonBody;
        google::protobuf::util::JsonParseOptions oParseOptions;
        google::protobuf::util::Status oStatus;
        if (stMsgHead.encript == 0)       // 未压缩也未加密
        {
            strJsonBody.resize(stMsgHead.body_len);
            strJsonBody.assign(pBuff->GetRawReadBuffer(), stMsgHead.body_len);
            oStatus = google::protobuf::util::JsonStringToMessage(strJsonBody, &oMsgBody, oParseOptions);
        }
        else    // 有压缩或加密，先解密再解压，然后用MsgBody反序列化
        {
            std::string strUncompressData;
            std::string strDecryptData;
            if (gc_uiRc5Bit & oMsgHead.cmd())
            {
                std::string strRawData;
                strRawData.assign((const char*) pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                if (!Rc5Decrypt(strRawData, strDecryptData))
                {
                    LOG4_ERROR("Rc5Decrypt error!");
                    return (CODEC_STATUS_ERR);
                }
            }
            if (gc_uiZipBit & oMsgHead.cmd())
            {
                if (strDecryptData.size() > 0)
                {
                    if (!Unzip(strDecryptData, strUncompressData))
                    {
                        LOG4_ERROR("uncompress error!");
                        return (CODEC_STATUS_ERR);
                    }
                }
                else
                {
                    std::string strRawData;
                    strRawData.assign((const char*) pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                    if (!Unzip(strRawData, strUncompressData))
                    {
                        LOG4_ERROR("uncompress error!");
                        return (CODEC_STATUS_ERR);
                    }
                }
            }
            else if (gc_uiGzipBit & oMsgHead.cmd())
            {
                if (strDecryptData.size() > 0)
                {
                    if (!Gunzip(strDecryptData, strUncompressData))
                    {
                        LOG4_ERROR("uncompress error!");
                        return (CODEC_STATUS_ERR);
                    }
                }
                else
                {
                    std::string strRawData;
                    strRawData.assign(
                            (const char*) pBuff->GetRawReadBuffer(),
                            stMsgHead.body_len);
                    if (!Gunzip(strRawData, strUncompressData))
                    {
                        LOG4_ERROR("uncompress error!");
                        return (CODEC_STATUS_ERR);
                    }
                }
            }

            if (strUncompressData.size() > 0)       // 解压后的数据
            {
                oStatus = google::protobuf::util::JsonStringToMessage(strDecryptData, &oMsgBody, oParseOptions);
                oMsgHead.set_len(oMsgBody.ByteSize());
            }
            else if (strDecryptData.size() > 0)     // 解密后的数据
            {
                oStatus = google::protobuf::util::JsonStringToMessage(strDecryptData, &oMsgBody, oParseOptions);
                oMsgHead.set_len(oMsgBody.ByteSize());
            }
            else    // 无效的压缩或解密算法，仍然解析原数据
            {
                strJsonBody.resize(stMsgHead.body_len);
                strJsonBody.assign(pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                oStatus = google::protobuf::util::JsonStringToMessage(strJsonBody, &oMsgBody, oParseOptions);
            }
        }
        if (oStatus.ok())
        {
            pBuff->SkipBytes(oMsgBody.ByteSize());
            return (CODEC_STATUS_OK);
        }
        else
        {
            LOG4_ERROR("cmd[%u], seq[%u] json string to MsgBody error!",
                    oMsgHead.cmd(), oMsgHead.seq());
            return (CODEC_STATUS_ERR);
        }
    }
    else
    {
        return (CODEC_STATUS_PAUSE);
    }
}

} /* namespace neb */
