/*******************************************************************************
 * Project:  Nebula
 * @file     CodecPrivate.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#include <netinet/in.h>
#include "logger/NetLogger.hpp"
#include "CodecPrivate.hpp"
#include "CodecDefine.hpp"

namespace neb
{

CodecPrivate::CodecPrivate(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType)
{
}

CodecPrivate::~CodecPrivate()
{
}

E_CODEC_STATUS CodecPrivate::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    LOG4_TRACE(" ");
    tagMsgHead stMsgHead;
    stMsgHead.version = 1;        // version暂时无用
    stMsgHead.encript = (unsigned char)(oMsgHead.cmd() >> 24);
    stMsgHead.cmd = htons((unsigned short)(gc_uiCmdBit & oMsgHead.cmd()));
    stMsgHead.body_len = htonl((unsigned int)oMsgHead.len());
    stMsgHead.seq = htonl(oMsgHead.seq());
    //stMsgHead.checksum = htons((unsigned short)stMsgHead.checksum);
    if (oMsgBody.ByteSize() > 1000000) // pb 最大限制
    {
        LOG4_ERROR("oMsgBody.ByteSize() > 1000000");
        return(CODEC_STATUS_ERR);
    }
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = sizeof(stMsgHead);
    LOG4_TRACE("cmd %u, seq %u, len %u", oMsgHead.cmd(), oMsgHead.seq(), oMsgHead.len());
    if (oMsgHead.len() == 0)    // 无包体（心跳包等）
    {
        iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        return(CODEC_STATUS_OK);
    }
    iHadWriteLen += iWriteLen;
    std::string strTmpData;
    if (stMsgHead.encript == 0)       // 不压缩也不加密
    {
        iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
        LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stMsgHead), iWriteLen);
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iNeedWriteLen = oMsgBody.ByteSize();
        oMsgBody.SerializeToString(&strTmpData);
        iWriteLen = pBuff->Write(strTmpData.c_str(), oMsgBody.ByteSize());
        if (iWriteLen != iNeedWriteLen)
        {
            LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
            return(CODEC_STATUS_ERR);
        }
        iHadWriteLen += iWriteLen;
    }
    else
    {
        std::string strCompressData;
        std::string strEncryptData;
        if (gc_uiZipBit & oMsgHead.cmd())
        {
            oMsgBody.SerializeToString(&strTmpData);
            if (!Zip(strTmpData, strCompressData))
            {
                LOG4_ERROR("zip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        else if (gc_uiGzipBit & oMsgHead.cmd())
        {
            oMsgBody.SerializeToString(&strTmpData);
            if (!Gzip(strTmpData, strCompressData))
            {
                LOG4_ERROR("gzip error!");
                return(CODEC_STATUS_ERR);
            }
        }
        if (gc_uiRc5Bit & oMsgHead.cmd())
        {
            if (strCompressData.size() > 0)
            {
                if (!Rc5Encrypt(strCompressData, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                oMsgBody.SerializeToString(&strTmpData);
                if (!Rc5Encrypt(strTmpData, strEncryptData))
                {
                    LOG4_ERROR("Rc5Encrypt error!");
                    return(CODEC_STATUS_ERR);
                }
            }
        }

        if (strEncryptData.size() > 0)              // 加密后的数据包
        {
            stMsgHead.body_len = htonl((unsigned int)strEncryptData.size());
            iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strEncryptData.size();
            iWriteLen = pBuff->Write(strEncryptData.c_str(), strEncryptData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else if (strCompressData.size() > 0)        // 压缩后的数据包
        {
            stMsgHead.body_len = htonl((unsigned int)strCompressData.size());
            iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = strCompressData.size();
            iWriteLen = pBuff->Write(strCompressData.c_str(), strCompressData.size());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
        else    // 无效的压缩或加密算法，打包原数据
        {
            iWriteLen = pBuff->Write(&stMsgHead, iNeedWriteLen);
            LOG4_TRACE("sizeof(stClientMsgHead) = %d, iWriteLen = %d", sizeof(stMsgHead), iWriteLen);
            if (iWriteLen != iNeedWriteLen)
            {
                LOG4_ERROR("buff write head iWriteLen != sizeof(stClientMsgHead)");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
            iNeedWriteLen = oMsgBody.ByteSize();
            oMsgBody.SerializeToString(&strTmpData);
            iWriteLen = pBuff->Write(strTmpData.c_str(), oMsgBody.ByteSize());
            if (iWriteLen != iNeedWriteLen)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
                return(CODEC_STATUS_ERR);
            }
            iHadWriteLen += iWriteLen;
        }
    }
    LOG4_TRACE("oMsgBody.ByteSize() = %d, iWriteLen = %d(compress or encrypt maybe)", oMsgBody.ByteSize(), iWriteLen);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecPrivate::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u", pBuff->ReadableBytes());
    size_t uiHeadSize = sizeof(tagMsgHead);
    if (pBuff->ReadableBytes() >= uiHeadSize)
    {
        tagMsgHead stMsgHead;
        int iReadIdx = pBuff->GetReadIndex();
        pBuff->Read(&stMsgHead, uiHeadSize);
        stMsgHead.cmd = ntohs(stMsgHead.cmd);
        stMsgHead.body_len = ntohl(stMsgHead.body_len);
        stMsgHead.seq = ntohl(stMsgHead.seq);
        stMsgHead.checksum = ntohs(stMsgHead.checksum);
        LOG4_TRACE("cmd %u, seq %u, len %u, pBuff->ReadableBytes() %u",
                        stMsgHead.cmd, stMsgHead.seq, stMsgHead.body_len,
                        pBuff->ReadableBytes());
        oMsgHead.set_cmd(((unsigned int)stMsgHead.encript << 24) | stMsgHead.cmd);
        oMsgHead.set_len(stMsgHead.body_len);
        oMsgHead.set_seq(stMsgHead.seq);
        if (0 == stMsgHead.body_len)      // 心跳包无包体
        {
            return(CODEC_STATUS_OK);
        }
        if (pBuff->ReadableBytes() >= stMsgHead.body_len)
        {
            bool bResult = false;
            if (stMsgHead.encript == 0)       // 未压缩也未加密
            {
                bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), stMsgHead.body_len);
            }
            else    // 有压缩或加密，先解密再解压，然后用MsgBody反序列化
            {
                std::string strUncompressData;
                std::string strDecryptData;
                if (gc_uiRc5Bit & oMsgHead.cmd())
                {
                    std::string strRawData;
                    strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                    if (!Rc5Decrypt(strRawData, strDecryptData))
                    {
                        LOG4_ERROR("Rc5Decrypt error!");
                        return(CODEC_STATUS_ERR);
                    }
                }
                if (gc_uiZipBit & oMsgHead.cmd())
                {
                    if (strDecryptData.size() > 0)
                    {
                        if (!Unzip(strDecryptData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                        if (!Unzip(strRawData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
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
                            return(CODEC_STATUS_ERR);
                        }
                    }
                    else
                    {
                        std::string strRawData;
                        strRawData.assign((const char*)pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                        if (!Gunzip(strRawData, strUncompressData))
                        {
                            LOG4_ERROR("uncompress error!");
                            return(CODEC_STATUS_ERR);
                        }
                    }
                }

                if (strUncompressData.size() > 0)       // 解压后的数据
                {
                    oMsgHead.set_len(strUncompressData.size());
                    bResult = oMsgBody.ParseFromString(strUncompressData);
                }
                else if (strDecryptData.size() > 0)     // 解密后的数据
                {
                    oMsgHead.set_len(strDecryptData.size());
                    bResult = oMsgBody.ParseFromString(strDecryptData);
                }
                else    // 无效的压缩或解密算法，仍然解析原数据
                {
                    bResult = oMsgBody.ParseFromArray(pBuff->GetRawReadBuffer(), stMsgHead.body_len);
                }
            }
            if (bResult)
            {
                pBuff->SkipBytes(oMsgBody.ByteSize());
                return(CODEC_STATUS_OK);
            }
            else
            {
                LOG4_ERROR("cmd[%u], seq[%u] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                return(CODEC_STATUS_ERR);
            }
        }
        else
        {
            pBuff->SetReadIndex(iReadIdx);
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

} /* namespace neb */
