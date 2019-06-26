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
#include "SocketChannelSslImpl.hpp"

namespace neb
{

SSL_CTX* SocketChannelSslImpl::s_pServerSslCtx = NULL;
SSL_CTX* SocketChannelSslImpl::s_pClientSslCtx = NULL;

SocketChannelSslImpl::SocketChannelSslImpl(
    SocketChannel* pSocketChannel, std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, ev_tstamp dKeepAlive)
    : SocketChannelImpl(pSocketChannel, pLogger, iFd, ulSeq, dKeepAlive),
      m_eSslChannelStatus(SSL_CHANNEL_INIT), m_bIsClientConnection(false), m_pSslConnection(NULL)
{
}

SocketChannelSslImpl::~SocketChannelSslImpl()
{
    LOG4_DEBUG("SocketChannelSslImpl::~SocketChannelSslImpl() fd %d, seq %u", GetFd(), GetSequence());
}

int SocketChannelSslImpl::SslInit(std::shared_ptr<NetLogger> pLogger)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100003L

    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "OPENSSL_init_ssl() failed!");
        return(ERR_SSL_INIT);
    }

    /*
     * OPENSSL_init_ssl() may leave errors in the error queue
     * while returning success
     */

    ERR_clear_error();

#else

    OPENSSL_config(NULL);

    SSL_library_init();
    SSL_load_error_strings();

    OpenSSL_add_all_algorithms();

#endif

    return(ERR_OK);
}

int SocketChannelSslImpl::SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger)
{
    pLogger->WriteLog(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__, " ");
    s_pServerSslCtx = SSL_CTX_new(TLS_server_method());

    if (s_pServerSslCtx == NULL)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_new() failed");
        return(ERR_SSL_CTX);
    }

    SSL_CTX_set_min_proto_version(s_pServerSslCtx, TLS1_1_VERSION);

#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_TLS_D5_BUG
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(s_pServerSslCtx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(s_pServerSslCtx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
    SSL_CTX_set_mode(s_pServerSslCtx, SSL_MODE_NO_AUTO_CHAIN);
#endif

    SSL_CTX_set_read_ahead(s_pServerSslCtx, 1);

    return(ERR_OK);
}

int SocketChannelSslImpl::SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
    const std::string& strCertFile, const std::string& strKeyFile)
{
    pLogger->WriteLog(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
            "SslServerCertificate(%s, %s)", strCertFile.c_str(), strKeyFile.c_str());
    // 加载使用公钥证书
    if (!SSL_CTX_use_certificate_chain_file(s_pServerSslCtx, strCertFile.c_str()))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_user_certificate_chain_file(\"%s\") failed!", strCertFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    }

    // 加载使用私钥
    if (!SSL_CTX_use_PrivateKey_file(s_pServerSslCtx, strKeyFile.c_str(), SSL_FILETYPE_PEM))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_use_PrivateKey_file(\"%s\") failed!", strKeyFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    }

    // 检查私钥与证书是否匹配
    if (!SSL_CTX_check_private_key(s_pServerSslCtx))  
    {  
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_check_private_key() failed: private key does not match the certificate public key!", strKeyFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    } 
    return(ERR_OK);
}

void SocketChannelSslImpl::SslFree()
{
    if (s_pServerSslCtx)
    {
        SSL_CTX_free(s_pServerSslCtx);
        s_pServerSslCtx = NULL;
    }
    if (s_pClientSslCtx)
    {
        SSL_CTX_free(s_pClientSslCtx);
        s_pClientSslCtx = NULL;
    }
}

int SocketChannelSslImpl::SslClientCtxCreate()
{
    if (s_pClientSslCtx == NULL)
    {
        s_pClientSslCtx = SSL_CTX_new(TLS_client_method());
        if (s_pClientSslCtx == NULL)
        {
            LOG4_ERROR("SSL_CTX_new() failed!");
            return(ERR_SSL_CTX);
        }
    }
    return(ERR_OK);
}

int SocketChannelSslImpl::SslCreateConnection()
{
    if (m_bIsClientConnection)
    {
        m_pSslConnection = SSL_new(s_pClientSslCtx);
    }
    else
    {
        m_pSslConnection = SSL_new(s_pServerSslCtx);
    }

    if (m_pSslConnection == NULL)
    {
        LOG4_ERROR("SSL_new() failed!");
        return(ERR_SSL_NEW_CONNECTION);
    }

    if (!SSL_set_fd(m_pSslConnection, GetFd()))
    {
        LOG4_ERROR("SSL_set_fd() failed!");
        return(ERR_SSL_NEW_CONNECTION);
    }

    if (m_bIsClientConnection)
    {
        SSL_set_connect_state(m_pSslConnection);
    }
    else
    {
        SSL_set_accept_state(m_pSslConnection);
    }

    return(ERR_OK);
}

int SocketChannelSslImpl::SslHandshake()
{
    LOG4_TRACE("");
    int iHandshakeResult = SSL_do_handshake(m_pSslConnection);
    if (iHandshakeResult == 1)
    {
        LOG4_TRACE("fd %d ssl handshake was successful.", GetFd());
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

int SocketChannelSslImpl::SslShutdown()
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
        LOG4_TRACE("fd %d ssl shutdown was successful.", GetFd());
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

    if (m_bIsClientConnection)
    {
        SSL_CTX_remove_session(s_pClientSslCtx, SSL_get0_session(m_pSslConnection));
    }
    else
    {
        SSL_CTX_remove_session(s_pServerSslCtx, SSL_get0_session(m_pSslConnection));
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

bool SocketChannelSslImpl::Init(E_CODEC_TYPE eCodecType, bool bIsClient)
{
    if (!SocketChannelImpl::Init(eCodecType, bIsClient))
    {
        return(false);
    }
    if (bIsClient)
    {
        m_bIsClientConnection = true;
        if (ERR_OK != SslClientCtxCreate())
        {
            return(false);
        }
        LOG4_TRACE("SslClientCtxCreate() successfully.");
    }
    if (ERR_OK != SslCreateConnection())
    {
        return(false);
    }
    LOG4_TRACE("SslCreateConnection() successfully.");
    return(true);
}

E_CODEC_STATUS SocketChannelSslImpl::Send()
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Send());
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl::Send());
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

E_CODEC_STATUS SocketChannelSslImpl::Send(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Send(iCmd, uiSeq, oMsgBody));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl::Send(iCmd, uiSeq, oMsgBody));
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    uint8 ucChannelStatus = GetChannelStatus();
                    SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl::Send(iCmd, uiSeq, oMsgBody);
                    SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    uint8 ucChannelStatus = GetChannelStatus();
                    SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl::Send(iCmd, uiSeq, oMsgBody);
                    SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
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

E_CODEC_STATUS SocketChannelSslImpl::Send(const HttpMsg& oHttpMsg, uint32 ulStepSeq)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Send(oHttpMsg, ulStepSeq));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl::Send(oHttpMsg, ulStepSeq));
                }
                else if (m_eSslChannelStatus == SSL_CHANNEL_WANT_READ)
                {
                    uint8 ucChannelStatus = GetChannelStatus();
                    SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl::Send(oHttpMsg, ulStepSeq);
                    SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
                    return(CODEC_STATUS_WANT_READ);
                }
                else 
                {
                    uint8 ucChannelStatus = GetChannelStatus();
                    SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
                    SocketChannelImpl::Send(oHttpMsg, ulStepSeq);
                    SetChannelStatus((E_CHANNEL_STATUS)ucChannelStatus);
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

E_CODEC_STATUS SocketChannelSslImpl::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Recv(oMsgHead, oMsgBody));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    if (m_bIsClientConnection)
                    {
                        return(CODEC_STATUS_WANT_WRITE);
                    }
                    else
                    {
                        return(SocketChannelImpl::Recv(oMsgHead, oMsgBody));
                    }
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

E_CODEC_STATUS SocketChannelSslImpl::Recv(HttpMsg& oHttpMsg)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Recv(oHttpMsg));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    if (m_bIsClientConnection)
                    {
                        return(CODEC_STATUS_WANT_WRITE);
                    }
                    else
                    {
                        return(SocketChannelImpl::Recv(oHttpMsg));
                    }
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

E_CODEC_STATUS SocketChannelSslImpl::Recv(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg)
{
    LOG4_TRACE("");
    switch (m_eSslChannelStatus)
    {
        case SSL_CHANNEL_ESTABLISHED:
            return(SocketChannelImpl::Recv(oMsgHead, oMsgBody, oHttpMsg));
        case SSL_CHANNEL_WANT_READ:
        case SSL_CHANNEL_WANT_WRITE:
        case SSL_CHANNEL_INIT:
            if (ERR_OK == SslHandshake())
            {
                if (m_eSslChannelStatus == SSL_CHANNEL_ESTABLISHED)
                {
                    return(SocketChannelImpl::Recv(oMsgHead, oMsgBody, oHttpMsg));
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

bool SocketChannelSslImpl::Close()
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
    return(SocketChannelImpl::Close());
}

int SocketChannelSslImpl::Write(CBuffer* pBuff, int& iErrno)
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
                LOG4_DEBUG("ssl channel(fd %d) closed by peer.", GetFd());
                break;
            default:
                LOG4_ERROR("SSL_write() error code %d, see SSL_get_error() manual for error code detail.", iErrCode);
                ;
        }
    }
    return(iWritenLen);
}

int SocketChannelSslImpl::Read(CBuffer* pBuff, int& iErrno)
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
                LOG4_DEBUG("ssl channel(fd %d) closed by peer.", GetFd());
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

}

#endif   // ifdef WITH_OPENSSL
