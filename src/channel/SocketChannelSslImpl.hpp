/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelSslImpl.hpp
 * @brief    SSL通信通道
 * @author   Bwar
 * @date:    2018年6月21日
 * @note     SSL Socket通信通道
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_
#define SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_

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

#include "SocketChannelImpl.hpp"

namespace neb 
{
    
class SocketChannelSslImpl : public SocketChannelImpl 
{
public:
    SocketChannelSslImpl();
    virtual ~SocketChannelSslImpl();

    static int SslInit(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
                const std::string& strCertFile, const std::string& strKeyFile);
    static int SslFree();

    int SslCreateConnection();
    int SslHandshake();
    int SslShutdown();

protected:
    virtual int Write(CBuffer* pBuff, int& iErrno);
    virtual int Read(CBuffer* pBuff, int& iErrno);

private: 
    static int m_iSslConnectionIndex;
    static int m_iSslServerConfIndex;
    static int m_iSslSessionCacheIndex;
    static int m_iSslSessionTicketKeysIndex;
    static int m_iSslCertificateIndex;
    static int m_iSslNextCertificateIndex;
    static int m_iSslCertificateNameIndex;
    static int m_iSslStaplingIndex;

    static SSL_CTX* m_pServerSslCtx;
    SSL_CTX* m_pClientSslCtx;

    SSL* m_pSslConnection;
};

}

#endif  // SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_
