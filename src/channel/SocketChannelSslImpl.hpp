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

#include "SocketChannelImpl.hpp"

namespace neb 
{

enum E_SSL_CHANNEL_STATUS
{
    SSL_CHANNEL_INIT        = 0,        ///< SslCreateConnection()
    SSL_CHANNEL_HANDSHAKE   = 1,        ///< SslHandshake() in progress
    SSL_CHANNEL_ESTABLISHED = 2,        ///< SslHanshaked() done
    SSL_CHANNEL_SHUTDOWN    = 3,        ///< SslShutDown() done
};
    
class SocketChannelSslImpl : public SocketChannelImpl 
{
public:
    SocketChannelSslImpl(SocketChannel* pSocketChannel, std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, ev_tstamp dKeepAlive = 0.0);
    virtual ~SocketChannelSslImpl();

    static int SslInit(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCtxCreate(std::shared_ptr<NetLogger> pLogger);
    static int SslServerCertificate(std::shared_ptr<NetLogger> pLogger,
                const std::string& strCertFile, const std::string& strKeyFile);
    static void SslFree();

    int SslClientCtxCreate();
    int SslCreateConnection();
    int SslHandshake();
    int SslShutdown();

    virtual bool Init(E_CODEC_TYPE eCodecType, bool bIsServer = true, const std::string& strKey = "That's a lizard.");
    virtual E_CODEC_STATUS Send();      ///< 覆盖基类的Send()方法，实现非阻塞socket连接建立后继续建立SSL连接
    virtual void Abort();

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
    E_SSL_CHANNEL_STATUS m_eSslChannelStatus;

    static SSL_CTX* m_pServerSslCtx;
    SSL_CTX* m_pClientSslCtx;

    SSL* m_pSslConnection;
};

}

#endif  // ifdef WITH_OPENSSL
#endif  // SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_
