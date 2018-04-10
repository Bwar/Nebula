/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannel.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include <labor/WorkerImpl.hpp>
#include "codec/CodecProto.hpp"
#include "codec/CodecPrivate.hpp"
#include "codec/CodecHttp.hpp"
#include "labor/Labor.hpp"
#include "labor/Manager.hpp"
#include "util/logger/NetLogger.hpp"
#include "SocketChannel.hpp"

namespace neb
{

SocketChannel::SocketChannel(std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq)
    : m_ucChannelStatus(0), m_iFd(iFd), m_ulSeq(ulSeq), m_ulStepSeq(0),
      m_ulForeignSeq(0), m_ulUnitTimeMsgNum(0), m_ulMsgNum(0), m_dActiveTime(0.0), m_dKeepAlive(0.0),
      m_pIoWatcher(nullptr), m_pTimerWatcher(nullptr),
      m_pRecvBuff(nullptr), m_pSendBuff(nullptr), m_pWaitForSendBuff(nullptr),
      m_pCodec(nullptr), m_iErrno(0), m_pLabor(nullptr), m_pLogger(pLogger)
{
    memset(m_szErrBuff, 0, sizeof(m_szErrBuff));
}

SocketChannel::~SocketChannel()
{
    close(m_iFd);
    FREE(m_pIoWatcher);
    FREE(m_pTimerWatcher);
    DELETE(m_pRecvBuff);
    DELETE(m_pSendBuff);
    DELETE(m_pWaitForSendBuff);
    DELETE(m_pCodec);
}

bool SocketChannel::Init(E_CODEC_TYPE eCodecType, const std::string& strKey)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(codec_type[%d])", __FUNCTION__, eCodecType);
    try
    {
        m_pRecvBuff = new CBuffer();
        m_pSendBuff = new CBuffer();
        m_pWaitForSendBuff = new CBuffer();
        switch (eCodecType)
        {
            case CODEC_PROTOBUF:
                m_pCodec = new CodecProto(m_pLogger, eCodecType, strKey);
                break;
            case CODEC_PRIVATE:
                m_pCodec = new CodecPrivate(m_pLogger, eCodecType, strKey);
                break;
            case CODEC_HTTP:
                m_pCodec = new CodecHttp(m_pLogger, eCodecType, strKey);
                break;
            default:
                m_pLogger->WriteLog(Logger::ERROR, "no codec defined for code type %d", eCodecType);
                break;
        }
    }
    catch(std::bad_alloc& e)
    {
        m_pLogger->WriteLog(Logger::ERROR, "%s", e.what());
        return(false);
    }
    m_strKey = strKey;
    m_dActiveTime = m_pLabor->GetNowTime();
    return(true);
}

E_CODEC_TYPE SocketChannel::GetCodecType() const
{
    return(m_pCodec->GetCodecType());
}

ev_tstamp SocketChannel::GetKeepAlive()
{
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
    }
    return(m_dKeepAlive);
}

bool SocketChannel::NeedAliveCheck() const
{
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        return(false);
    }
    return(true);
}

E_CODEC_STATUS SocketChannel::Send()
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    else if (CHANNEL_STATUS_ESTABLISHED != m_ucChannelStatus)
    {
        return(CODEC_STATUS_PAUSE);
    }
    int iNeedWriteLen = 0;
    int iWriteLen = 0;
    m_pLogger->WriteLog(Logger::TRACE, "m_pSendBuff = 0x%d, m_pSendBuff->ReadableBytes() = %d", m_pSendBuff, m_pSendBuff->ReadableBytes());
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
            m_pLogger->WriteLog(Logger::ERROR, "send to fd %d error %d: %s",
                    m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
            m_strErrMsg = m_szErrBuff;
            return(CODEC_STATUS_INT);
        }
    }
    else if (m_pWaitForSendBuff->ReadableBytes() > 0)
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
        m_pLogger->WriteLog(Logger::ERROR, "send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Send(uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d], cmd[%u], seq[%u])",
                    __FUNCTION__, m_iFd, m_ulSeq, uiCmd, uiSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    int32 iMsgBodyLen = oMsgBody.ByteSize();
    MsgHead oMsgHead;
    oMsgHead.set_cmd(uiCmd);
    oMsgHead.set_seq(uiSeq);
    iMsgBodyLen = (iMsgBodyLen > 0) ? iMsgBodyLen : -1;     // proto3里int赋值为0会在指定固定大小的message时有问题
    oMsgHead.set_len(iMsgBodyLen);
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
            switch (uiCmd)
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
            m_pLogger->WriteLog(Logger::ERROR, "invalid connect status %d!", m_ucChannelStatus);
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
    m_pLogger->WriteLog(Logger::TRACE, "iNeedWriteLen = %d, iWriteLen = %d", iNeedWriteLen, iWriteLen);
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
        m_pLogger->WriteLog(Logger::ERROR, "send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Send(const HttpMsg& oHttpMsg, uint32 ulStepSeq)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
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
            m_pLogger->WriteLog(Logger::ERROR, "invalid connect status %d!", m_ucChannelStatus);
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
        m_pLogger->WriteLog(Logger::ERROR, "send to fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    m_pLogger->WriteLog(Logger::TRACE, "recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
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
                    m_pLogger->WriteLog(Logger::ERROR, "invalid connect status %d!", m_ucChannelStatus);
                    return(CODEC_STATUS_OK);
            }
        }
        m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u], cmd[%u], seq[%u])", __FUNCTION__, m_iFd, m_ulSeq, oMsgHead.cmd(), oMsgHead.seq());
        return(eCodecStatus);
    }
    else if (iReadLen == 0)
    {
        m_pLogger->WriteLog(Logger::DEBUG, "fd %d closed by peer, error %d %s!",
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
        m_pLogger->WriteLog(Logger::ERROR, "recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Recv(HttpMsg& oHttpMsg)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    m_pLogger->WriteLog(Logger::TRACE, "recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
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
        m_pLogger->WriteLog(Logger::DEBUG, "fd %d closed by peer, error %d %s!",
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
        m_pLogger->WriteLog(Logger::ERROR, "recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    int iReadLen = 0;
    iReadLen = m_pRecvBuff->ReadFD(m_iFd, m_iErrno);
    m_pLogger->WriteLog(Logger::TRACE, "recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
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
                m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u], cmd[%u], seq[%u])", __FUNCTION__, m_iFd, m_ulSeq, oMsgHead.cmd(), oMsgHead.seq());
            }
            return(eCodecStatus);
        }
    }
    else if (iReadLen == 0)
    {
        m_pLogger->WriteLog(Logger::DEBUG, "fd %d closed by peer, error %d %s!",
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
        m_pLogger->WriteLog(Logger::ERROR, "recv from fd %d error %d: %s",
                m_iFd, m_iErrno, strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff)));
        m_strErrMsg = m_szErrBuff;
        return(CODEC_STATUS_INT);
    }
}

E_CODEC_STATUS SocketChannel::Fetch(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    m_pLogger->WriteLog(Logger::TRACE, "fetch from fd %d and m_pRecvBuff->ReadableBytes() = %d",
            m_iFd, m_pRecvBuff->ReadableBytes());
    E_CODEC_STATUS eCodecStatus = m_pCodec->Decode(m_pRecvBuff, oMsgHead, oMsgBody);
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        m_ulForeignSeq = oMsgHead.seq();
        ++m_ulUnitTimeMsgNum;
        ++m_ulMsgNum;
        m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u], cmd[%u], seq[%u])", __FUNCTION__, m_iFd, m_ulSeq, oMsgHead.cmd(), oMsgHead.seq());
    }
    return(eCodecStatus);
}

E_CODEC_STATUS SocketChannel::Fetch(HttpMsg& oHttpMsg)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
    E_CODEC_STATUS eCodecStatus = ((CodecHttp*)m_pCodec)->Decode(m_pRecvBuff, oHttpMsg);
    m_dKeepAlive = ((CodecHttp*)m_pCodec)->GetKeepAlive();
    return(eCodecStatus);
}

E_CODEC_STATUS SocketChannel::Fetch(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d])", __FUNCTION__, m_iFd, m_ulSeq);
    if (CHANNEL_STATUS_DISCARD == m_ucChannelStatus || CHANNEL_STATUS_DESTROY == m_ucChannelStatus)
    {
        return(CODEC_STATUS_EOF);
    }
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
            m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%u], cmd[%u], seq[%u])", __FUNCTION__, m_iFd, m_ulSeq, oMsgHead.cmd(), oMsgHead.seq());
        }
        return(eCodecStatus);
    }
}

bool SocketChannel::SwitchCodec(E_CODEC_TYPE eCodecType, ev_tstamp dKeepAlive)
{
    m_pLogger->WriteLog(Logger::TRACE, "%s(channel_fd[%d], channel_id[%d], codec_type[%d], new_codec_type[%d])",
                    __FUNCTION__, m_iFd, m_ulSeq, m_pCodec->GetCodecType(), eCodecType);
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
                pNewCodec = new CodecProto(m_pLogger, eCodecType, m_strKey);
                break;
            case CODEC_PRIVATE:
                pNewCodec = new CodecPrivate(m_pLogger, eCodecType, m_strKey);
                break;
            case CODEC_HTTP:
                pNewCodec = new CodecHttp(m_pLogger, eCodecType, m_strKey);
                break;
            default:
                m_pLogger->WriteLog(Logger::ERROR, "no codec defined for code type %d", eCodecType);
                break;
        }
    }
    catch(std::bad_alloc& e)
    {
        m_pLogger->WriteLog(Logger::ERROR, "%s", e.what());
        return(false);
    }
    DELETE(m_pCodec);
    m_pCodec = pNewCodec;
    m_dKeepAlive = dKeepAlive;
    m_dActiveTime = m_pLabor->GetNowTime();
    return(true);
}

ev_io* SocketChannel::AddIoWatcher()
{
    m_pIoWatcher = (ev_io*)malloc(sizeof(ev_io));
    if (NULL != m_pIoWatcher)
    {
        m_pIoWatcher->data = this;      // (void*)(Channel*)
    }
    return(m_pIoWatcher);
}

ev_timer* SocketChannel::AddTimerWatcher()
{
    m_pTimerWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (NULL != m_pTimerWatcher)
    {
        m_pTimerWatcher->data = this;    // (void*)(Channel*)
    }
    return(m_pTimerWatcher);
}

} /* namespace neb */
