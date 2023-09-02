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
#include "logger/NetLogger.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/SocketChannelImpl.hpp"

namespace neb
{

CodecProto::CodecProto(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
        std::shared_ptr<SocketChannel> pBindChannel)
    : Codec(pLogger, eCodecType, pBindChannel)
{
}

CodecProto::~CodecProto()
{
}

int CodecProto::Write(uint32 uiCodecType, uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, MsgHead&& oMsgHead, MsgBody&& oMsgBody)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<MsgBody, MsgHead>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(uiCodecType, uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<MsgBody, MsgHead>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, uiCodecType);
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(uiCodecType, uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

int CodecProto::Write(uint32 uiCodecType, std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, MsgBody&& oMsgBody)
{
    uint32 uiFrom;
    uint32 uiTo;
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiCodecType, uiTo, uiFrom, uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody)));
}


E_CODEC_STATUS CodecProto::Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecProto::Encode(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    std::string strTmpData;
    MsgHead oMsgHead;
    int32 iMsgBodyLen = oMsgBody.ByteSize();
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    iMsgBodyLen = (iMsgBodyLen > 0) ? iMsgBodyLen : -1;
    oMsgHead.set_len(iMsgBodyLen);
    oMsgHead.SerializeToString(&strTmpData);
    iWriteLen = pBuff->Write(strTmpData.c_str(), gc_uiMsgHeadSize);
    if (iWriteLen != gc_uiMsgHeadSize)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    iHadWriteLen += iWriteLen;
    if (oMsgHead.len() <= 0)    // 无包体（心跳包等），nebula在proto3的使用上以-1表示包体长度为0
    {
        return(ChannelSticky(oMsgHead, oMsgBody));
    }
    oMsgBody.SerializeToString(&strTmpData);
    iWriteLen = pBuff->Write(strTmpData.c_str(), iMsgBodyLen);
    iHadWriteLen += iWriteLen;
    if (iWriteLen == iMsgBodyLen)
    {
        return(ChannelSticky(oMsgHead, oMsgBody));
    }
    else
    {
        LOG4_ERROR("buff write body iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS CodecProto::Encode(int32 iCmd, uint32 uiSeq,
        const MsgBody& oMsgBody, CBuffer* pBuff, CBuffer* pSecondlyBuff)
{
    if (CHANNEL_STATUS_ESTABLISHED == GetBindChannel()->GetChannelStatus())
    {
        return(Encode(iCmd, uiSeq, oMsgBody, pBuff));
    }
    switch (iCmd)
    {
        case CMD_RSP_TELL_WORKER:
            return(Encode(iCmd, uiSeq, oMsgBody, pBuff));
        case CMD_REQ_TELL_WORKER:
            return(Encode(iCmd, uiSeq, oMsgBody, pBuff));
        case CMD_RSP_CONNECT_TO_WORKER:
            return(Encode(iCmd, uiSeq, oMsgBody, pBuff));
        case CMD_REQ_CONNECT_TO_WORKER:
            return(Encode(iCmd, uiSeq, oMsgBody, pBuff));
        default:
            return(Encode(iCmd, uiSeq, oMsgBody, pSecondlyBuff));
    }
}

E_CODEC_STATUS CodecProto::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("pBuff->ReadableBytes()=%d, pBuff->GetReadIndex()=%d",
                    pBuff->ReadableBytes(), pBuff->GetReadIndex());
    if (pBuff->ReadableBytes() >= gc_uiMsgHeadSize)
    {
        bool bResult = oMsgHead.ParseFromArray(pBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
        if (bResult)
        {
            LOG4_TRACE("pBuff->ReadableBytes()=%d, oMsgHead.len()=%d",
                            pBuff->ReadableBytes(), oMsgHead.len());
            if (oMsgHead.len() <= 0)      // 无包体（心跳包等），nebula在proto3的使用上以-1表示包体长度为0
            {
                oMsgHead.set_len(0);
                pBuff->SkipBytes(gc_uiMsgHeadSize);
                return(ChannelSticky(oMsgHead, oMsgBody));
            }
            if (pBuff->ReadableBytes() >= gc_uiMsgHeadSize + oMsgHead.len())
            {
                bResult = oMsgBody.ParseFromArray(
                                pBuff->GetRawReadBuffer() + gc_uiMsgHeadSize, oMsgHead.len());
                LOG4_TRACE("pBuff->ReadableBytes()=%d, oMsgBody.ByteSize()=%d", pBuff->ReadableBytes(), oMsgBody.ByteSize());
                if (bResult)
                {
                    pBuff->SkipBytes(gc_uiMsgHeadSize + oMsgHead.len());
                    oMsgBody.set_add_on(GetBindChannel()->GetClientData());
                    return(ChannelSticky(oMsgHead, oMsgBody));
                }
                else
                {
                    LOG4_WARNING("cmd[%u], seq[%u] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
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
            LOG4_TRACE("oMsgHead.ParseFromArray() failed!");   // maybe port scan from operation and maintenance system.
            return(CODEC_STATUS_ERR);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

E_CODEC_STATUS CodecProto::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody, CBuffer* pReactBuff)
{
    LOG4_ERROR("invalid");
    return(CODEC_STATUS_ERR);
}

E_CODEC_STATUS CodecProto::ChannelSticky(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    switch (GetBindChannel()->GetChannelStatus())
    {
        case CHANNEL_STATUS_ESTABLISHED:
            if ((gc_uiCmdReq & oMsgHead.cmd()) && (GetBindChannel()->GetClientData().size() > 0))
            {
                m_uiForeignSeq = oMsgHead.seq();
            }
            break;
        case CHANNEL_STATUS_CLOSED:
        case CHANNEL_STATUS_BROKEN:
            LOG4_WARNING("%s channel_fd[%d], channel_status[%d] remote %s EOF.",
                    GetBindChannel()->GetIdentify().c_str(), GetBindChannel()->GetFd(),
                    (int)GetBindChannel()->GetChannelStatus(), GetBindChannel()->GetRemoteAddr().c_str());
            return(CODEC_STATUS_ERR);
        case CHANNEL_STATUS_TELL_WORKER:
        case CHANNEL_STATUS_WORKER:
        case CHANNEL_STATUS_TRANSFER_TO_WORKER:
        case CHANNEL_STATUS_CONNECTED:
        case CHANNEL_STATUS_TRY_CONNECT:
        case CHANNEL_STATUS_INIT:
        {
            auto pChannelImpl = std::static_pointer_cast<SocketChannelImpl<CodecNebula>>(GetBindChannel()->m_pImpl);
            switch (oMsgHead.cmd())
            {
                case CMD_RSP_TELL_WORKER:
                    pChannelImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
                    break;
                case CMD_REQ_TELL_WORKER:
                    pChannelImpl->SetChannelStatus(CHANNEL_STATUS_TELL_WORKER);
                    break;
                case CMD_RSP_CONNECT_TO_WORKER:
                    pChannelImpl->SetChannelStatus(CHANNEL_STATUS_WORKER);
                    break;
                case CMD_REQ_CONNECT_TO_WORKER:
                    pChannelImpl->SetChannelStatus(CHANNEL_STATUS_TRANSFER_TO_WORKER);
                    break;
                default:
                    if (CODEC_PROTO == GetCodecType())
                    {
                        LOG4_TRACE("cmd[%d] channel_fd[%d], channel_seq[%d], channel_status[from %d to %d] may be a fault.",
                                oMsgHead.cmd(), pChannelImpl->GetFd(), pChannelImpl->GetSequence(),
                                (int)pChannelImpl->GetChannelStatus(), CHANNEL_STATUS_ESTABLISHED);
                        pChannelImpl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
                    }
                    break;
            }
        }
            break;
        default:
            LOG4_ERROR("%s invalid connection status %d!", GetBindChannel()->GetIdentify().c_str(),
                    (int)GetBindChannel()->GetChannelStatus());
            return(CODEC_STATUS_ERR);
    }
    return(CODEC_STATUS_OK);
}

} /* namespace neb */
