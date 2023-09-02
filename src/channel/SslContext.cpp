/*******************************************************************************
 * Project:  Nebula
 * @file     SslContext.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-04
 * @note     
 * Modify history:
 ******************************************************************************/
#include "SslContext.hpp"

#ifdef WITH_OPENSSL

namespace neb
{

SSL_CTX* SslContext::s_pServerSslCtx = nullptr;
SSL_CTX* SslContext::s_pClientSslCtx = nullptr;

SslContext::SslContext()
{
    // TODO Auto-generated constructor stub

}

SslContext::~SslContext()
{
    // TODO Auto-generated destructor stub
}

int SslContext::SslInit(std::shared_ptr<NetLogger> pLogger)
{
    if (s_pServerSslCtx != nullptr)
    {
        return(ERR_OK);
    }
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

int SslContext::SslClientCtxCreate(std::shared_ptr<NetLogger> pLogger)
{
    if (s_pClientSslCtx == nullptr)
    {
        s_pClientSslCtx = SSL_CTX_new(TLS_client_method());
        if (s_pClientSslCtx == nullptr)
        {
            pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "SSL_CTX_new() failed!");
            return(ERR_SSL_CTX);
        }
    }
    return(ERR_OK);
}

int SslContext::SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger)
{
    pLogger->WriteLog(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__, " ");
    if (s_pServerSslCtx != nullptr)
    {
        return(ERR_OK);
    }
    s_pServerSslCtx = SSL_CTX_new(TLS_server_method());

    if (s_pServerSslCtx == nullptr)
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

int SslContext::SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
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

void SslContext::SslFree()
{
    if (s_pServerSslCtx)
    {
        SSL_CTX_free(s_pServerSslCtx);
        s_pServerSslCtx = nullptr;
    }
    if (s_pClientSslCtx)
    {
        SSL_CTX_free(s_pClientSslCtx);
        s_pClientSslCtx = nullptr;
    }
}

} /* namespace neb */

#endif  // #ifdef WITH_OPENSSL

