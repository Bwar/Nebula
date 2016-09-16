/*******************************************************************************
 * Project:  Nebula
 * @file     CodecProto.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CodecProto.hpp"

namespace neb
{

CodecProto::CodecProto(E_CODEC_TYPE eCodecType, const std::string& strKey)
    : Codec(eCodecType, strKey)
{
}

CodecProto::~CodecProto()
{
}

E_CODEC_STATUS CodecProto::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes()=%u", __FUNCTION__, pBuff->ReadableBytes());
    int iErrno = 0;
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = oMsgHead.ByteSize();
    iWriteLen = pBuff->Write(oMsgHead.SerializeAsString().c_str(), oMsgHead.ByteSize());
    if (iWriteLen != iNeedWriteLen)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    iHadWriteLen += iWriteLen;
    if (oMsgHead.len() == 0)    // 无包体（心跳包等）
    {
        return(CODEC_STATUS_OK);
    }
    iNeedWriteLen = oMsgBody.ByteSize();
    iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
    LOG4_TRACE("pBuff->ReadableBytes()=%u", pBuff->ReadableBytes());
    if (iWriteLen == iNeedWriteLen)
    {
        return(CODEC_STATUS_OK);
    }
    else
    {
        LOG4_ERROR("buff write body iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS CodecProto::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("%s() pBuff->ReadableBytes()=%d, pBuff->GetReadIndex()=%d",
                    __FUNCTION__, pBuff->ReadableBytes(), pBuff->GetReadIndex());
    oMsgHead.set_cmd(0);
    oMsgHead.set_len(0);
    oMsgHead.set_seq(0);
    int iHeadSize = oMsgHead.ByteSize();
    if ((int)(pBuff->ReadableBytes()) >= iHeadSize)
    {
        bool bResult = oMsgHead.ParseFromArray(pBuff->GetRawReadBuffer(), iHeadSize);
        if (bResult)
        {
            if (0 == oMsgHead.len())      // 无包体（心跳包等）
            {
                pBuff->SkipBytes(iHeadSize);
                return(CODEC_STATUS_OK);
            }
            if (pBuff->ReadableBytes() >= iHeadSize + oMsgHead.len())
            {
                bResult = oMsgBody.ParseFromArray(
                                pBuff->GetRawReadBuffer() + iHeadSize, oMsgHead.len());
                if (bResult)
                {
                    pBuff->SkipBytes(oMsgHead.ByteSize() + oMsgBody.ByteSize());
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("cmd[%u], seq[%lu] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                return(CODEC_STATUS_PAUSE);
            }
        }
        else
        {
            LOG4_ERROR("oMsgHead.ParseFromArray() error!");
            return(CODEC_STATUS_ERR);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

} /* namespace neb */
