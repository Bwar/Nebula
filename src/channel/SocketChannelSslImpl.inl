/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelSslImpl.cpp
 * @brief    SSL通信通道
 * @author   Bwar
 * @date:    2018年6月21日
 * @note     SSL Socket通信通道
 * Modify history:
 ******************************************************************************/

#ifdef WITH_OPENSSL

namespace neb
{

template <typename T>
SocketChannelSslImpl<T>::SocketChannelSslImpl(
    Labor* pLabor, std::shared_ptr<NetLogger> pLogger,
    bool bIsClient, bool bWithSsl, int iFd, uint32 ulSeq, ev_tstamp dKeepAlive)
    : SocketChannelImpl<T>(pLabor, pLogger, bIsClient, bWithSsl, iFd, ulSeq, dKeepAlive),
      m_eSslChannelStatus(SSL_CHANNEL_INIT), m_pSslConnection(NULL)
{
}

template <typename T>
SocketChannelSslImpl<T>::~SocketChannelSslImpl()
{
    LOG4_DEBUG("SocketChannelSslImpl<T>::~SocketChannelSslImpl() fd %d, seq %u",
            SocketChannelImpl<T>::GetFd(), SocketChannelImpl<T>::GetSequence());
}

template <typename T>
int SocketChannelSslImpl<T>::SslCreateConnection()
{
    if (SocketChannelImpl<T>::IsClient())
    {
        m_pSslConnection = SSL_new(SslContext::ClientSslCtx());
    }
    else
    {
        m_pSslConnection = SSL_new(SslContext::ServerSslCtx());
    }

    if (m_pSslConnection == NULL)
    {
        LOG4_ERROR("SSL_new() failed!");
        return(ERR_SSL_NEW_CONNECTION);
    }

    if (!SSL_set_fd(m_pSslConnection, SocketChannelImpl<T>::GetFd()))
    {
        LOG4_ERROR("SSL_set_fd() failed!");
        return(ERR_SSL_NEW_CONNECTION);
    }

    if (SocketChannelImpl<T>::IsClient())
    {
        SSL_set_connect_state(m_pSslConnection);
    }
    else
    {
        SSL_set_accept_state(m_pSslConnection);
    }

    return(ERR_OK);
}

template <typename T>
int SocketChannelSslImpl<T>::SslHandshake()
{
    LOG4_TRACE("");
    int iHandshakeResult = SSL_do_handshake(m_pSslConnection);
    if (iHandshakeResult == 1)
    {
        LOG4_TRACE("fd %d ssl handshake was successful.", SocketChannelImpl<T>::GetFd());
        m_eSslChannelStatus = SSL_CHANNEL_ESTABLISHED;
        return(ERR_OK);
    }
    else    // 0 and < 0
    {
        int iErrCode = SSL_get_error(m_pSslConnection, iHandshakeResult);
        LOG4_DEBUG("iErrCode = %d", iErrCode);
        switch (iErrCode)
        {
            case SSL_ERROR_ZERO_RETURN:
                LOG4_ERROR("The TLS/SSL connection has been closed. "
                    "If the protocol version is SSL 3.0 or higher, this result code is returned "
                    "only if a closure alert has occurred in the protocol, "
                    "i.e. if the connection has been closed cleanly. "
                    "Note that in this case SSL_ERROR_ZERO_RETURN does not necessarily indicate "
                    "that the underlying transport has been closed.");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_WANT_READ:
                LOG4_DEBUG("SSL_ERROR_WANT_READ: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                m_eSslChannelStatus = SSL_CHANNEL_WANT_READ;
                break;
            case SSL_ERROR_WANT_WRITE:
                LOG4_DEBUG("SSL_ERROR_WANT_WRITE: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                m_eSslChannelStatus = SSL_CHANNEL_WANT_WRITE;
                break;
            case SSL_ERROR_WANT_CONNECT: // SSL_do_handshake() will never get this
                LOG4_DEBUG("SSL_ERROR_WANT_CONNECT: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_WANT_ACCEPT: // SSL_do_handshake() will never get this
                LOG4_DEBUG("SSL_ERROR_WANT_ACCEPT: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_WANT_X509_LOOKUP: // SSL_do_handshake() will never get this
                LOG4_DEBUG("The operation did not complete because an application callback set by SSL_CTX_set_client_cert_cb() "
                    "has asked to be called again. The TLS/SSL I/O function should be called again later.");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_WANT_ASYNC: // SSL_do_handshake() will never get this 
                // This will only occur if the mode has been set to SSL_MODE_ASYNC using SSL_CTX_set_mode or SSL_set_mode and an asynchronous capable engine is being used. 
                LOG4_WARNING("The operation did not complete because an asynchronous engine is still processing data. ");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_WANT_ASYNC_JOB: // SSL_do_handshake() will never get this 
                // This will only occur if the mode has been set to SSL_MODE_ASYNC using SSL_CTX_set_mode or SSL_set_mode and a maximum limit has been set on the async job pool through a call to ASYNC_init_thread. 
                LOG4_WARNING("The asynchronous job could not be started because there were no async jobs available in the pool (see ASYNC_init_thread(3)).");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_SYSCALL:
                if (EINTR == errno)
                {
                    return(ERR_OK);
                }
                else
                {
                    LOG4_ERROR("Some non-recoverable I/O error occurred. The OpenSSL error queue may contain more information on the error. "
                        "For socket I/O on Unix systems, consult errno %d for details.", errno);
                    return(ERR_SSL_HANDSHAKE);
                }
            case SSL_ERROR_SSL:
                LOG4_ERROR("A failure in the SSL library occurred, usually a protocol error. The OpenSSL error queue contains more information on the error.");
                return(ERR_SSL_HANDSHAKE);
            case SSL_ERROR_NONE:    // if and only if iHandshakeResult > 0
                break;
            default:
                ;
        } 
    }
    return(ERR_OK);
}

template <typename T>
int SocketChannelSslImpl<T>::SslShutdown()
{
    if (SSL_in_init(m_pSslConnection))
    {
        /*
         * OpenSSL 1.0.2f complains if SSL_shutdown() is called during
         * an SSL handshake, while previous versions always return 0.
         * Avoid calling SSL_shutdown() if handshake wasn't completed.
         */
        m_eSslChannelStatus = SSL_CHANNEL_SHUTDOWN;
        SSL_free(m_pSslConnection);
        m_pSslConnection = NULL;
        
        ERR_clear_error();
#if OPENSSL_VERSION_NUMBER < 0x10100003L
        EVP_cleanup();
#ifndef OPENSSL_NO_ENGINE
        ENGINE_cleanup();
#endif
#endif
        return(ERR_OK);
    }

    // int iShutdownResult = 0;
    // int iShutdownMode = 0;
    // if (c->timedout)
    // {
    //     iShutdownMode = SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN;
    //     SSL_set_quiet_shutdown(m_pSslConnection, 1);
    // }
    // else
    // {
    //     iShutdownMode = SSL_get_shutdown(m_pSslConnection);
    //     if (c->ssl->no_wait_shutdown)
    //     {
    //         iShutdownMode |= SSL_RECEIVED_SHUTDOWN;
    //     }

    //     if (no_send_shutdown)
    //     {
    //         iShutdownMode |= SSL_SENT_SHUTDOWN;
    //     }

    //     if (c->ssl->no_wait_shutdown && c->ssl->no_send_shutdown)
    //     {
    //         SSL_set_quiet_shutdown(m_pSslConnection, 1);
    //     }
    // }

    // SSL_set_shutdown(m_pSslConnection, iShutdownMode);

    int iShutdownResult = SSL_shutdown(m_pSslConnection);
    if (iShutdownResult == 1)
    {
        LOG4_TRACE("fd %d ssl shutdown was successful.", SocketChannelImpl<T>::GetFd());
        m_eSslChannelStatus = SSL_CHANNEL_SHUTDOWN;
    }
    else    // 0 and < 0
    {
        int iErrCode = SSL_get_error(m_pSslConnection, iShutdownResult);
        LOG4_DEBUG("iErrCode = %d", iErrCode);
        switch (iErrCode)
        {
            case SSL_ERROR_ZERO_RETURN:
                LOG4_ERROR("The TLS/SSL connection has been closed. "
                    "If the protocol version is SSL 3.0 or higher, this result code is returned "
                    "only if a closure alert has occurred in the protocol, "
                    "i.e. if the connection has been closed cleanly. "
                    "Note that in this case SSL_ERROR_ZERO_RETURN does not necessarily indicate "
                    "that the underlying transport has been closed.");
                break;
            case SSL_ERROR_WANT_READ:
                LOG4_DEBUG("SSL_ERROR_WANT_READ: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                m_eSslChannelStatus = SSL_CHANNEL_SHUTING_WANT_READ;
                return(ERR_OK);
            case SSL_ERROR_WANT_WRITE:
                LOG4_DEBUG("SSL_ERROR_WANT_WRITE: The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                m_eSslChannelStatus = SSL_CHANNEL_SHUTING_WANT_WRITE;
                return(ERR_OK);
            case SSL_ERROR_SYSCALL:
                if (errno != 0)
                {
                    LOG4_DEBUG("Some non-recoverable I/O error occurred. The OpenSSL error queue may contain more information on the error. "
                        "For socket I/O on Unix systems, consult errno %d for details.", errno);
                }
                break;
            case SSL_ERROR_SSL:
                LOG4_ERROR("A failure in the SSL library occurred, usually a protocol error. The OpenSSL error queue contains more information on the error.");
                break;
            default:
                ;
        }
        m_eSslChannelStatus = SSL_CHANNEL_SHUTDOWN;
    }

    if (SocketChannelImpl<T>::IsClient())
    {
        SSL_CTX_remove_session(SslContext::ClientSslCtx(), SSL_get0_session(m_pSslConnection));
    }
    else
    {
        SSL_CTX_remove_session(SslContext::ServerSslCtx(), SSL_get0_session(m_pSslConnection));
    }

    SSL_free(m_pSslConnection);
    m_pSslConnection = NULL;

    ERR_clear_error();
    CRYPTO_cleanup_all_ex_data();
#if OPENSSL_VERSION_NUMBER < 0x10100003L
    EVP_cleanup();
#ifndef OPENSSL_NO_ENGINE
    ENGINE_cleanup();
#endif
#endif

    return(ERR_OK);
}

template <typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelSslImpl<T>::SendRequest(uint32 uiStepSeq, Targs&&... args)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl<T>::SendRequest(uiStepSeq, std::forward<Targs>(args)...));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl<T>::SendRequest(uiStepSeq, std::forward<Targs>(args)...));
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    uint8 ucChannelStatus = SocketChannelImpl<T>::GetChannelStatus();
                    SocketChannelImpl<T>::SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl<T>::SendRequest(uiStepSeq, std::forward<Targs>(args)...);
                    SocketChannelImpl<T>::SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    uint8 ucChannelStatus = SocketChannelImpl<T>::GetChannelStatus();
                    SocketChannelImpl<T>::SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl<T>::SendRequest(uiStepSeq, std::forward<Targs>(args)...);
                    SocketChannelImpl<T>::SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTING_WANT_READ:
        case SSL_CHANNEL_SHUTING_WANT_WRITE:
            if (ERR_OK == SslShutdown())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_SHUTDOWN)
                {
                    return(CODEC_STATUS_EOF);
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTDOWN:
            LOG4_ERROR("the ssl channel had been shutdown!");
            return(CODEC_STATUS_ERR);
        default:
            LOG4_ERROR("invalid ssl channel status!");
            return(CODEC_STATUS_ERR);
    }
}

template <typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelSslImpl<T>::SendResponse(Targs&& ...args)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl<T>::SendResponse(std::forward<Targs>(args)...));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl<T>::SendResponse(std::forward<Targs>(args)...));
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    uint8 ucChannelStatus = SocketChannelImpl<T>::GetChannelStatus();
                    SocketChannelImpl<T>::SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl<T>::SendResponse(std::forward<Targs>(args)...);
                    SocketChannelImpl<T>::SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    uint8 ucChannelStatus = SocketChannelImpl<T>::GetChannelStatus();
                    SocketChannelImpl<T>::SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl<T>::SendResponse(std::forward<Targs>(args)...);
                    SocketChannelImpl<T>::SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTING_WANT_READ:
        case SSL_CHANNEL_SHUTING_WANT_WRITE:
            if (ERR_OK == SslShutdown())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_SHUTDOWN)
                {
                    return(CODEC_STATUS_EOF);
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTDOWN:
            LOG4_ERROR("the ssl channel had been shutdown!");
            return(CODEC_STATUS_ERR);
        default:
            LOG4_ERROR("invalid ssl channel status!");
            return(CODEC_STATUS_ERR);
    }
}

template <typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelSslImpl<T>::Recv(Targs&& ...args)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl<T>::Recv(std::forward<Targs>(args)...));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    if (SocketChannelImpl<T>::IsClient())
                    {
                        return(CODEC_STATUS_WANT_WRITE);
                    }
                    else
                    {
                        return(SocketChannelImpl<T>::Recv(std::forward<Targs>(args)...));
                    }
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    Send();
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTING_WANT_READ:
        case SSL_CHANNEL_SHUTING_WANT_WRITE:
            if (ERR_OK == SslShutdown())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_SHUTDOWN)
                {
                    return(CODEC_STATUS_EOF);
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    Send();
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTDOWN:
            LOG4_ERROR("the ssl channel had been shutdown!");
            return(CODEC_STATUS_ERR);
        default:
            LOG4_ERROR("invalid ssl channel status!");
            return(CODEC_STATUS_ERR);
    }
}

template <typename T>
E_CODEC_STATUS SocketChannelSslImpl<T>::Send()
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl<T>::Send());
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl<T>::Send());
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTING_WANT_READ:
        case SSL_CHANNEL_SHUTING_WANT_WRITE:
            if (ERR_OK == SslShutdown())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_SHUTDOWN)
                {
                    return(CODEC_STATUS_EOF);
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_READ)
                {
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    return(CODEC_STATUS_WANT_WRITE);
                }
            }
            else
            {
                return(CODEC_STATUS_ERR);
            }
            break;
        case SSL_CHANNEL_SHUTDOWN:
            LOG4_ERROR("the ssl channel had been shutdown!");
            return(CODEC_STATUS_ERR);
        default:
            LOG4_ERROR("invalid ssl channel status!");
            return(CODEC_STATUS_ERR);
    }
}

template <typename T>
bool SocketChannelSslImpl<T>::Close()
{
    if (m_eSslChannelStatus != SSL_CHANNEL_SHUTDOWN)
    {
        SslShutdown();
        if (m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_READ
                || m_eSslChannelStatus == SSL_CHANNEL_SHUTING_WANT_WRITE)
        {
            return(false);
        }
    }
    return(SocketChannelImpl<T>::Close());
}

template <typename T>
int SocketChannelSslImpl<T>::Write(CBuffer* pBuff, int& iErrno)
{
    LOG4_TRACE("");
    int iNeedWriteLen = (int)pBuff->ReadableBytes();
    int iWritenLen = SSL_write(m_pSslConnection, pBuff->GetRawReadBuffer(), iNeedWriteLen);
    if (iWritenLen > 0)
    {
        pBuff->AdvanceReadIndex(iWritenLen);
    }
    else
    {
        iErrno = errno;
        int iErrCode = SSL_get_error(m_pSslConnection, iWritenLen);
        switch (iErrCode)
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                LOG4_DEBUG("The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                iErrno = EAGAIN;
                break;
            case SSL_ERROR_ZERO_RETURN:
                LOG4_DEBUG("ssl channel(fd %d) closed by peer.", SocketChannelImpl<T>::GetFd());
                break;
            default:
                LOG4_ERROR("SSL_write() error code %d, see SSL_get_error() manual for error code detail.", iErrCode);
                ;
        }
    }
    return(iWritenLen);
}

template <typename T>
int SocketChannelSslImpl<T>::Read(CBuffer* pBuff, int& iErrno)
{
    LOG4_TRACE("");
    if (pBuff->WriteableBytes() < 1024)
    {
        pBuff->EnsureWritableBytes(8192);
    }
    int iReadLen = SSL_read(m_pSslConnection, pBuff->GetRawWriteBuffer(), pBuff->WriteableBytes());
    if (iReadLen > 0)
    {
        pBuff->AdvanceWriteIndex(iReadLen);
    }
    else
    {
        iErrno = errno;
        int iErrCode = SSL_get_error(m_pSslConnection, iReadLen);
        switch (iErrCode)
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                LOG4_DEBUG("The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                iErrno = EAGAIN;
                break;
            case SSL_ERROR_ZERO_RETURN:
                LOG4_DEBUG("ssl channel(fd %d) closed by peer.", SocketChannelImpl<T>::GetFd());
                break;
            case SSL_ERROR_SYSCALL:
                LOG4_DEBUG("Some non-recoverable I/O error occurred. The OpenSSL error queue may contain more information on the error. "
                    "For socket I/O on Unix systems, consult errno %d for details.", errno);
                break;
            default:
                LOG4_ERROR("SSL_read() error code %d, see SSL_get_error() manual for error code detail.", iErrCode);
                ;
        }
    }
    return(iReadLen);
}

template <typename T>
template <typename ...Targs>
void SocketChannelSslImpl<T>::Logger(int32 iLogLevel, const char* szFileName, uint32 uiFileLine, const char* szFunction, Targs&&... args)
{
    SocketChannelImpl<T>::Logger(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

}

#endif   // ifdef WITH_OPENSSL
