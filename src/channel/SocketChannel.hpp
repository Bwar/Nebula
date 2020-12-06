/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannel.hpp
 * @brief    通信通道
 * @author   Bwar
 * @date:    2016年8月10日
 * @note     通信通道，如Socket连接
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SOCKETCHANNEL_HPP_
#define SRC_CHANNEL_SOCKETCHANNEL_HPP_

#include "SocketChannelImpl.hpp"
#ifdef WITH_OPENSSL
#include "SocketChannelSslImpl.hpp"
#endif

namespace neb
{

class Dispatcher;

class SocketChannel: public Channel, public std::enable_shared_from_this<SocketChannel>
{
public:
    struct tagChannelCtx
    {
        int iFd;
        int iAiFamily;        // AF_INET  or   AF_INET6
        int iCodecType;
    };

    SocketChannel(std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, bool bWithSsl = false, ev_tstamp dKeepAlive = 10.0);
    virtual ~SocketChannel();
    
    static int SendChannelFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType, std::shared_ptr<NetLogger> pLogger);
    static int RecvChannelFd(int iSocketFd, int& iRecvFd, int& iAiFamily, int& iCodecType, std::shared_ptr<NetLogger> pLogger);

    virtual bool Init(E_CODEC_TYPE eCodecType, bool bIsClient = false);

    int GetFd() const;
    bool IsClient() const
    {
        return(m_bIsClientConnection);
    }
    bool IsPipeline() const;
    const std::string& GetIdentify() const;
    const std::string& GetRemoteAddr() const;
    const std::string& GetClientData() const;
    E_CODEC_TYPE GetCodecType() const;

private:
    bool m_bIsClientConnection;
    // Hide most of the channel implementation for Actors
    // 以m_pImpl对Actor及其派生类隐藏框架层才需要的Channel大部分实现
    //std::unique_ptr<SocketChannelImpl> m_pImpl;
    std::shared_ptr<SocketChannelImpl> m_pImpl;
    std::shared_ptr<NetLogger> m_pLogger;
    
    friend class Dispatcher;
    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_SOCKETCHANNEL_HPP_ */

