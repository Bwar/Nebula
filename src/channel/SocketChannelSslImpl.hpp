/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelSslImpl.hpp
 * @brief    SSL Socket通信通道实现
 * @author   Bwar
 * @date:    2018年6月21日
 * @note     SSL Socket通信通道实现
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
    SSL_CHANNEL_INIT                    = 0,        ///< SslCreateConnection()
    SSL_CHANNEL_WANT_READ               = 1,        ///< SslHandshake() in progress, see SSL_ERROR_WANT_READ for detail
    SSL_CHANNEL_WANT_WRITE              = 2,        ///< SslHandshake() in progress, see SSL_ERROR_WANT_WRITE for detail
    SSL_CHANNEL_ESTABLISHED             = 3,        ///< SslHanshaked() done
    SSL_CHANNEL_SHUTING_WANT_READ       = 4,        ///< SslShutDown() in progress
    SSL_CHANNEL_SHUTING_WANT_WRITE      = 5,        ///< SslShutDown() in progress
    SSL_CHANNEL_SHUTDOWN                = 6,        ///< SslShutDown() done
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

    virtual bool Init(E_CODEC_TYPE eCodecType, bool bIsClient = false) override;
    virtual E_CODEC_STATUS Send() override;      ///< 覆盖基类的Send()方法，实现非阻塞socket连接建立后继续建立SSL连接
    virtual E_CODEC_STATUS Send(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody) override;
    virtual E_CODEC_STATUS Send(const HttpMsg& oHttpMsg, uint32 ulStepSeq) override;
    virtual E_CODEC_STATUS Recv(MsgHead& oMsgHead, MsgBody& oMsgBody) override;
    virtual E_CODEC_STATUS Recv(HttpMsg& oHttpMsg) override;
    virtual E_CODEC_STATUS Recv(MsgHead& oMsgHead, MsgBody& oMsgBody, HttpMsg& oHttpMsg) override;
    virtual bool Close() override;

protected:
    virtual int Write(CBuffer* pBuff, int& iErrno) override;
    virtual int Read(CBuffer* pBuff, int& iErrno) override;

private: 
    E_SSL_CHANNEL_STATUS m_eSslChannelStatus;
    bool m_bIsClientConnection;
    SSL* m_pSslConnection;

    static SSL_CTX* s_pServerSslCtx;
    static SSL_CTX* s_pClientSslCtx;
};

}

#endif  // ifdef WITH_OPENSSL
#endif  // SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_
