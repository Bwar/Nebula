/*******************************************************************************
 * Project:  Nebula
 * @file     Channel.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include "codec/CodecProto.hpp"
#include "codec/CodecPrivate.hpp"
#include "codec/CodecHttp.hpp"
#include "labor/Labor.hpp"
#include "Channel.hpp"

namespace neb
{

Channel::Channel(int iFd, uint32 ulSeq)
    : m_ucChannelStatus(0), m_iFd(iFd), m_ulSeq(ulSeq), m_ulStepSeq(0),
      m_ulForeignSeq(0), m_ulUnitTimeMsgNum(0), m_ulMsgNum(0), m_dActiveTime(0.0), m_dKeepAlive(0.0),
      m_pIoWatcher(NULL), m_pTimerWatcher(NULL),
      m_pRecvBuff(NULL), m_pSendBuff(NULL), m_pWaitForSendBuff(NULL),
      m_pCodec(NULL), m_iErrno(0),
      m_pLogger(NULL), m_pLabor(NULL)
{
    memset(m_szErrBuff, 0, sizeof(m_szErrBuff));
}

Channel::~Channel()
{
    close(m_iFd);
    FREE(m_pIoWatcher);
    FREE(m_pTimerWatcher);
    DELETE(m_pRecvBuff);
    DELETE(m_pSendBuff);
    DELETE(m_pWaitForSendBuff);
    DELETE(m_pCodec);
}

bool Channel::Init(E_CODEC_TYPE eCodecType, const std::string& strKey)
{
    // NEW_PTR(m_pRecvBuff, CBuffer);
    try
    {
        m_pRecvBuff = new CBuffer();
        m_pSendBuff = new CBuffer();
        m_pWaitForSendBuff = new CBuffer();
        switch (eCodecType)
        {
            case CODEC_PROTOBUF:
                m_pCodec = new CodecProto(eCodecType, strKey);
                break;
            case CODEC_PRIVATE:
                m_pCodec = new CodecPrivate(eCodecType, strKey);
                break;
            case CODEC_HTTP:
                m_pCodec = new CodecHttp(eCodecType, strKey);
                break;
            default:
                LOG4_ERROR("no codec defined for code type %d", eCodecType);
                break;
        }
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
        return(false);
    }
    m_strKey = strKey;
    m_dActiveTime = m_pLabor->GetNowTime();
    return(true);
}

E_CODEC_TYPE Channel::GetCodecType() const
{
    return(m_pCodec->GetCodecType());
}

ev_tstamp Channel::GetKeepAlive()
{
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
    }
    return(m_dKeepAlive);
}

bool Channel::NeedAliveCheck() const
{
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        return(false);
    }
    return(true);
}

E_CODEC_STATUS Channel::Send()
{
    int iNeedWriteLen = 0;
    int iWriteLen = 0;
    iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (iNeedWriteLen > 0)
    {
        iWriteLen = m_pSendBuff->WriteFD(m_iFd, m_iErrno);
        if (iWriteLen >= 0)
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            if (iNeedWriteLen == iWriteLen)
            {
                CBuffer* pExchangeBuff = m_pSendBuff;
                m_pSendBuff = m_pWaitForSendBuff;
                m_pWaitForSendBuff = pExchangeBuff;
            }
            else
            {
                iNeedWriteLen = m_pWaitForSendBuff->ReadableBytes();
                iWriteLen += m_pSendBuff->Write(m_pWaitForSendBuff, iNeedWriteLen - iWriteLen);
                return(CODEC_STATUS_PAUSE);
            }
        }
        else
        {
            if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
            {
                iNeedWriteLen = m_pWaitForSendBuff->ReadableBytes();
                iWriteLen += m_pSendBuff->Write(m_pWaitForSendBuff, iNeedWriteLen - iWriteLen);
                m_dActiveTime = m_pLabor->GetNowTime();
                return(CODEC_STATUS_PAUSE);
            }
            LOG4_ERROR("send to fd %d error %d: %s",
                    m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
            m_strErrMsg = m_szErrBuff;
            return(CODEC_STATUS_INT);
        }
    }
    else
    {
        CBuffer* pExchangeBuff = m_pSendBuff;
        m_pSendBuff = m_pWaitForSendBuff;
        m_pWaitForSendBuff = pExchangeBuff;
    }

    iNeedWriteLen = m_pSendBuff->ReadableBytes();
    iWriteLen = m_pSendBuff->WriteFD(m_iFd, m_iErrno);
    if (iWriteLen >= 0)
    {
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iWriteLen)
        {
            return(CODEC_STATUS_OK);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Send(const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    switch (m_ucChannelStatus)
    {
        case CHANNEL_STATUS_ESTABLISHED:
            eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pSendBuff);
            break;
        case CHANNEL_STATUS_TELL_WORKER:
        case CHANNEL_STATUS_WORKER:
        case CHANNEL_STATUS_TRANSFER_TO_WORKER:
        case CHANNEL_STATUS_CONNECTED:
        case CHANNEL_STATUS_TRY_CONNECT:
        case CHANNEL_STATUS_INIT:
        {
            switch (oMsgHead.cmd())
            {
                case CMD_RSP_TELL_WORKER:
                    m_ucChannelStatus = CHANNEL_STATUS_ESTABLISHED;
                    eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pSendBuff);
                    break;
                case CMD_REQ_TELL_WORKER:
                    m_ucChannelStatus = CHANNEL_STATUS_TELL_WORKER;
                    eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pSendBuff);
                    break;
                case CMD_RSP_CONNECT_TO_WORKER:
                    m_ucChannelStatus = CHANNEL_STATUS_WORKER;
                    eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pSendBuff);
                    break;
                case CMD_REQ_CONNECT_TO_WORKER:
                    m_ucChannelStatus = CHANNEL_STATUS_TRANSFER_TO_WORKER;
                    eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pSendBuff);
                    break;
                default:
                    eCodecStatus = m_pCodec->Encode(oMsgHead, oMsgBody, m_pWaitForSendBuff);
                    if (CODEC_STATUS_OK == eCodecStatus)
                    {
                        eCodecStatus = CODEC_STATUS_PAUSE;
                    }
                    break;
            }
        }
            break;
        default:
            LOG4_ERROR("invalid connect status %d!", m_ucChannelStatus);
            return(CODEC_STATUS_OK);
    }

    if (CODEC_STATUS_OK != eCodecStatus)
    {
        return(eCodecStatus);
    }

    int iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (iNeedWriteLen <= 0)
    {
        return(eCodecStatus);
    }

    int iWriteLen = m_pSendBuff->WriteFD(m_iFd, m_iErrno);
    if (iWriteLen >= 0)
    {
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iWriteLen)
        {
            return(CODEC_STATUS_OK);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Send(const HttpMsg& oHttpMsg, uint32 ulStepSeq)
{
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    switch (m_ucChannelStatus)
    {
        case CHANNEL_STATUS_ESTABLISHED:
            eCodecStatus = ((CodecHttp*)m_pCodec)->Encode(oHttpMsg, m_pSendBuff);
            m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
            break;
        case CHANNEL_STATUS_TELL_WORKER:
        case CHANNEL_STATUS_WORKER:
        case CHANNEL_STATUS_TRANSFER_TO_WORKER:
        case CHANNEL_STATUS_CONNECTED:
        case CHANNEL_STATUS_TRY_CONNECT:
        case CHANNEL_STATUS_INIT:
            eCodecStatus = ((CodecHttp*)m_pCodec)->Encode(oHttpMsg, m_pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                eCodecStatus = CODEC_STATUS_PAUSE;
            }
            break;
        default:
            LOG4_ERROR("invalid connect status %d!", m_ucChannelStatus);
            return(CODEC_STATUS_OK);
    }

    if (CODEC_STATUS_OK != eCodecStatus)
    {
        return(eCodecStatus);
    }

    int iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (iNeedWriteLen <= 0)
    {
        return(eCodecStatus);
    }

    int iWriteLen = m_pSendBuff->WriteFD(m_iFd, m_iErrno);
    if (iWriteLen >= 0)
    {
        m_ulStepSeq = ulStepSeq;
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iWriteLen)
        {
            return(CODEC_STATUS_OK);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    LOG4_TRACE("recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
            m_iFd, iReadLen, m_pRecvBuff->ReadableBytes());
    if (iReadLen > 0)
    {
        m_dActiveTime = m_pLabor->GetNowTime();
        E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pRecvBuff, oMsgHead, oMsgBody);
        if (CODEC_STATUS_OK == eCodecStatus)
        {
            switch (m_ucChannelStatus)
            {
                case CHANNEL_STATUS_ESTABLISHED:
                    if ((gc_uiCmdReq & oMsgHead.cmd()) && (m_strClientData.size() > 0))
                    {
                        m_ulForeignSeq = oMsgHead.seq();
                        ++m_ulUnitTimeMsgNum;
                        ++m_ulMsgNum;
                        oMsgBody.set_add_on(m_strClientData);
                    }
                    break;
                case CHANNEL_STATUS_TELL_WORKER:
                case CHANNEL_STATUS_WORKER:
                case CHANNEL_STATUS_TRANSFER_TO_WORKER:
                case CHANNEL_STATUS_CONNECTED:
                case CHANNEL_STATUS_TRY_CONNECT:
                case CHANNEL_STATUS_INIT:
                {
                    switch (oMsgHead.cmd())
                    {
                        case CMD_RSP_TELL_WORKER:
                            m_ucChannelStatus = CHANNEL_STATUS_ESTABLISHED;
                            break;
                        case CMD_REQ_TELL_WORKER:
                            m_ucChannelStatus = CHANNEL_STATUS_TELL_WORKER;
                            break;
                        case CMD_RSP_CONNECT_TO_WORKER:
                            m_ucChannelStatus = CHANNEL_STATUS_WORKER;
                            break;
                        case CMD_REQ_CONNECT_TO_WORKER:
                            m_ucChannelStatus = CHANNEL_STATUS_TRANSFER_TO_WORKER;
                            break;
                        default:
                            break;
                    }
                }
                    break;
                default:
                    LOG4_ERROR("invalid connect status %d!", m_ucChannelStatus);
                    return(CODEC_STATUS_OK);
            }
        }
        return(eCodecStatus);
    }
    else if (iReadLen == 0)
    {
        LOG4_DEBUG("fd %d closed by peer, error %d %s!",
                        m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_EOF);
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Recv(HttpMsg& oHttpMsg)
{
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    LOG4_TRACE("recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
            m_iFd, iReadLen, m_pRecvBuff->ReadableBytes());
    if (iReadLen > 0)
    {
        m_dActiveTime = m_pLabor->GetNowTime();
        E_CODEC_STATUS eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
        m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
        return(eCodecStatus);
    }
    else if (iReadLen == 0)
    {
        LOG4_DEBUG("fd %d closed by peer, error %d %s!",
                        m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        if (m_pRecvBuff->ReadableBytes() > 0)
        {
            E_CODEC_STATUS eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
            if (CODEC_STATUS_PAUSE == eCodecStatus || CODEC_STATUS_OK == eCodecStatus)
            {
                oHttpMsg.set_is_decoding(false);
            }
        }
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_EOF);
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg)
{
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    LOG4_TRACE("recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
            m_iFd, iReadLen, m_pRecvBuff->ReadableBytes());
    if (iReadLen > 0)
    {
        m_dActiveTime = m_pLabor->GetNowTime();
        if (CODEC_HTTP == m_pCodec->GetCodecType())
        {
            E_CODEC_STATUS eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
            m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
            return(eCodecStatus);
        }
        else
        {
            E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pRecvBuff, oMsgHead, oMsgBody);
            if ((CODEC_STATUS_OK == eCodecStatus) && (gc_uiCmdReq & oMsgHead.cmd()) && (m_strClientData.size() > 0))
            {
                m_ulForeignSeq = oMsgHead.seq();
                ++m_ulUnitTimeMsgNum;
                ++m_ulMsgNum;
                oMsgBody.set_add_on(m_strClientData);
            }
            return(eCodecStatus);
        }
    }
    else if (iReadLen == 0)
    {
        LOG4_DEBUG("fd %d closed by peer, error %d %s!",
                        m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_EOF);
    }
    else
    {
        if (EAGAIN == m_iErrno && EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        LOG4_ERROR("recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS Channel::Fetch(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pRecvBuff, oMsgHead, oMsgBody);
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        m_ulForeignSeq = oMsgHead.seq();
        ++m_ulUnitTimeMsgNum;
        ++m_ulMsgNum;
    }
    return(eCodecStatus);
}

E_CODEC_STATUS Channel::Fetch(HttpMsg& oHttpMsg)
{
    E_CODEC_STATUS eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
    m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
    return(eCodecStatus);
}

E_CODEC_STATUS Channel::Fetch(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg)
{
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
        m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
        return(eCodecStatus);
    }
    else
    {
        eCodecStatus = m_pCodec->Decode(m_pRecvBuff, oMsgHead, oMsgBody);
        if (CODEC_STATUS_OK == eCodecStatus)
        {
            m_ulForeignSeq = oMsgHead.seq();
            ++m_ulUnitTimeMsgNum;
            ++m_ulMsgNum;
        }
        return(eCodecStatus);
    }
}

bool Channel::SwitchCodec(E_CODEC_TYPE eCodecType, ev_tstamp dKeepAlive)
{
    if (eCodecType == m_pCodec->GetCodecType())
    {
        return(true);
    }

    Codec* pNewCodec = NULL;
    try
    {
        switch (eCodecType)
        {
            case CODEC_PROTOBUF:
                pNewCodec = new CodecProto(eCodecType, m_strKey);
                break;
            case CODEC_PRIVATE:
                pNewCodec = new CodecPrivate(eCodecType, m_strKey);
                break;
            case CODEC_HTTP:
                pNewCodec = new CodecHttp(eCodecType, m_strKey);
                break;
            default:
                LOG4_ERROR("no codec defined for code type %d", eCodecType);
                break;
        }
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
        return(false);
    }
    DELETE(m_pCodec);
    m_pCodec = pNewCodec;
    m_dKeepAlive = dKeepAlive;
    m_dActiveTime = m_pLabor->GetNowTime();
    return(true);
}

ev_io* Channel::AddIoWatcher()
{
    m_pIoWatcher = (ev_io*)malloc(sizeof(ev_io));
    if (NULL != m_pIoWatcher)
    {
        m_pIoWatcher->data = this;      // (void*)(Channel*)
    }
    return(m_pIoWatcher);
}

ev_timer* Channel::AddTimerWatcher()
{
    m_pTimerWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (NULL != m_pTimerWatcher)
    {
        m_pTimerWatcher->data = this;    // (void*)(Channel*)
    }
    return(m_pTimerWatcher);
}

} /* namespace neb */
