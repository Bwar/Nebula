/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelSslImpl.cpp
 * @brief    SSL通信通道
 * @author   Bwar
 * @date:    2018年6月21日
 * @note     SSL Socket通信通道
 * Modify history:
 ******************************************************************************/

#include "SocketChannelSslImpl.hpp"

namespace neb
{

int SocketChannelSslImpl::m_iSslConnectionIndex = -1;
int SocketChannelSslImpl::m_iSslServerConfIndex = -1;
int SocketChannelSslImpl::m_iSslSessionCacheIndex = -1;
int SocketChannelSslImpl::m_iSslSessionTicketKeysIndex = -1;
int SocketChannelSslImpl::m_iSslCertificateIndex = -1;
int SocketChannelSslImpl::m_iSslNextCertificateIndex = -1;
int SocketChannelSslImpl::m_iSslCertificateNameIndex = -1;
int SocketChannelSslImpl::m_iSslStaplingIndex = -1;
SSL_CTX* SocketChannelSslImpl::m_pServerSslCtx = NULL;

SocketChannelSslImpl::SocketChannelSslImpl()
     : m_pClientSslCtx(NULL), m_pSslConnection(NULL)
{
}

SocketChannelSslImpl::~SocketChannelSslImpl()
{
    if (m_pClientSslCtx)
    {
        SSL_CTX_free(m_pClientSslCtx);
    }
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

    m_iSslConnectionIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslConnectionIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslServerConfIndex = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslServerConfIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslSessionCacheIndex = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslSessionCacheIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslSessionTicketKeysIndex = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslSessionTicketKeysIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslCertificateIndex = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslCertificateIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslNextCertificateIndex = X509_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslNextCertificateIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "X509_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslCertificateNameIndex = X509_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslCertificateNameIndex == -1) {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "X509_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    m_iSslStaplingIndex = X509_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (m_iSslStaplingIndex == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "X509_get_ex_new_index() failed!");
        return(ERR_SSL_INIT);
    }

    return(ERR_OK);
}

int SocketChannelSslImpl::SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger)
{
    m_pServerSslCtx = SSL_CTX_new(TLS_server_method());

    if (m_pServerSslCtx == NULL)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_new() failed");
        return(ERR_SSL_CTX);
    }

    // if (SSL_CTX_set_ex_data(m_pServerSslCtx, m_iSslServerConfIndex, data) == 0)
    if (SSL_CTX_set_ex_data(m_pServerSslCtx, m_iSslServerConfIndex, NULL) == 0)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_set_ex_data() failed");
        return(ERR_SSL_CTX);
    }

    if (SSL_CTX_set_ex_data(m_pServerSslCtx, m_iSslCertificateIndex, NULL) == 0)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_set_ex_data() failed");
        return(ERR_SSL_CTX);
    }

    SSL_CTX_set_min_proto_version(m_pServerSslCtx, TLS1_1_VERSION);

#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_TLS_D5_BUG
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(m_pServerSslCtx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(m_pServerSslCtx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
    SSL_CTX_set_mode(m_pServerSslCtx, SSL_MODE_NO_AUTO_CHAIN);
#endif

    SSL_CTX_set_read_ahead(m_pServerSslCtx, 1);

    // SSL_CTX_set_info_callback(m_pServerSslCtx, ngx_ssl_info_callback);

    return(ERR_OK);
}

int SocketChannelSslImpl::SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
    const std::string& strCertFile, const std::string& strKeyFile)
{
    // 加载使用公钥证书
    if (!SSL_CTX_use_certificate_chain_file(m_pServerSslCtx, strCertFile.c_str()))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_user_certificate_chain_file(\"%s\") failed!", strCertFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    }

    // 加载使用私钥
    if (!SSL_CTX_use_PrivateKey_file(m_pServerSslCtx, strKeyFile.c_str(), SSL_FILETYPE_PEM))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_use_PrivateKey_file(\"%s\") failed!", strKeyFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    }

    // 检查私钥与证书是否匹配
    if (!SSL_CTX_check_private_key(m_pServerSslCtx))  
    {  
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
            "SSL_CTX_check_private_key() failed: private key does not match the certificate public key!", strKeyFile.c_str());
        return(ERR_SSL_CERTIFICATE);
    } 
}

int SocketChannelSslImpl::SslFree()
{
    if (m_pServerSslCtx)
    {
        SSL_CTX_free(m_pServerSslCtx);
    }
}

int SocketChannelSslImpl::SslCreateConnection()
{
    if (m_pClientSslCtx == NULL)
    {
        m_pSslConnection = SSL_new(m_pServerSslCtx);
    }
    else
    {
        m_pSslConnection = SSL_new(m_pClientSslCtx);
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

    if (m_pClientSslCtx == NULL)
    {
        SSL_set_accept_state(m_pSslConnection);
    }
    else
    {
        SSL_set_connect_state(m_pSslConnection);
    }

    return(ERR_OK);
}

int SocketChannelSslImpl::SslHandshake()
{
    int iHandshakeResult = SSL_do_handshake(m_pSslConnection);
    if (iHandshakeResult == 1)
    {
        LOG4_TRACE("fd %d ssl handshake was successful.", GetFd());
        return(ERR_OK);
    }
    else    // 0 and < 0
    {
        int iErrCode = SSL_get_error(m_pSslConnection, iHandshakeResult);
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
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
                LOG4_WARNING("The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                break;
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                LOG4_WARNING("The operation did not complete; the same TLS/SSL I/O function should be called again later.");
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                LOG4_WARNING("The operation did not complete because an application callback set by SSL_CTX_set_client_cert_cb() "
                    "has asked to be called again. The TLS/SSL I/O function should be called again later.");
                break;
            case SSL_ERROR_WANT_ASYNC:  // This will only occur if the mode has been set to SSL_MODE_ASYNC using SSL_CTX_set_mode or SSL_set_mode and an asynchronous capable engine is being used. 
                LOG4_WARNING("The operation did not complete because an asynchronous engine is still processing data. ");
                break;
            case SSL_ERROR_WANT_ASYNC_JOB: // This will only occur if the mode has been set to SSL_MODE_ASYNC using SSL_CTX_set_mode or SSL_set_mode and a maximum limit has been set on the async job pool through a call to ASYNC_init_thread. 
                LOG4_WARNING("The asynchronous job could not be started because there were no async jobs available in the pool (see ASYNC_init_thread(3)).");
                break;
            case SSL_ERROR_SYSCALL:
                LOG4_ERROR("Some non-recoverable I/O error occurred. The OpenSSL error queue may contain more information on the error. "
                    "For socket I/O on Unix systems, consult errno %d for details.", errno);
                break;
            case SSL_ERROR_SSL:
                LOG4_ERROR("A failure in the SSL library occurred, usually a protocol error. The OpenSSL error queue contains more information on the error.");
                break;
            case SSL_ERROR_NONE:    // if and only if iHandshakeResult > 0
                break;
            default:
        } 
    }
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
        SSL_free(m_pSslConnection);
        m_pSslConnection = NULL;
        return(ERR_OK);
    }

    int iShutdownResult = 0;
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

    iShutdownResult = SSL_shutdown(m_pSslConnection);

    SSL_free(m_pSslConnection);
    m_pSslConnection = NULL;

    return(ERR_OK);
}

}
