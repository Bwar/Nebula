/*******************************************************************************
 * Project:  Nebula
 * @file     SslContext.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-04
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SSLCONTEXT_HPP_
#define SRC_CHANNEL_SSLCONTEXT_HPP_

#ifdef WITH_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>
#include <openssl/dh.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
#include <openssl/evp.h>
#include <openssl/hmac.h>
#ifndef OPENSSL_NO_OCSP
#include <openssl/ocsp.h>
#endif
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "Error.hpp"
#include "Definition.hpp"
#include "logger/NetLogger.hpp"

namespace neb
{

class SslContext
{
public:
    SslContext();
    virtual ~SslContext();

    static int SslInit(std::shared_ptr<NetLogger> pLogger);
    static int SslClientCtxCreate(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
                const std::string& strCertFile, const std::string& strKeyFile);
    static void SslFree();

    static SSL_CTX* ServerSslCtx()
    {
        return(s_pServerSslCtx);
    }

    static SSL_CTX* ClientSslCtx()
    {
        return(s_pClientSslCtx);
    }

private:
    static SSL_CTX* s_pServerSslCtx;
    static SSL_CTX* s_pClientSslCtx;
};

} /* namespace neb */

#endif  // #ifdef WITH_OPENSSL

#endif /* SRC_CHANNEL_SSLCONTEXT_HPP_ */

