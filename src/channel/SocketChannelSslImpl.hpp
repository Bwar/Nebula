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
#include "SslContext.hpp"
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
    
template<typename T>
class SocketChannelSslImpl : public SocketChannelImpl<T>
{
public:
    SocketChannelSslImpl(Labor* pLabor, std::shared_ptr<NetLogger> pLogger,
            bool bIsClient, bool bWithSsl, int iFd, uint32 ulSeq, ev_tstamp dKeepAlive = 0.0);
    virtual ~SocketChannelSslImpl();

    int SslClientCtxCreate();
    int SslCreateConnection();
    int SslHandshake();
    int SslShutdown();

    template<typename ...Targs>
    E_CODEC_STATUS SendRequest(uint32 uiStepSeq, Targs&&... args);
    template<typename ...Targs>
    E_CODEC_STATUS SendResponse(Targs&&... args);
    template<typename ...Targs>
    E_CODEC_STATUS Recv(Targs&&... args);
    E_CODEC_STATUS Send();
    bool Close();
    template<typename ...Targs>
    void Logger(int iLogLevel, const char* szFileName, uint32 uiFileLine, const char* szFunction, Targs&&... args);

protected:
    virtual int Write(CBuffer* pBuff, int& iErrno) override;
    virtual int Read(CBuffer* pBuff, int& iErrno) override;

private: 
    E_SSL_CHANNEL_STATUS m_eSslChannelStatus;
    SSL* m_pSslConnection;
};

}

#endif  // ifdef WITH_OPENSSL
#include "SocketChannelSslImpl.inl"
#endif  // SRC_CHANNEL_SOCKETCHANNELSSLIMPL_HPP_

