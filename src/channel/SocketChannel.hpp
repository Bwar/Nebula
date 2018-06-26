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
#include "SocketChannelSslImpl.hpp"

namespace neb
{

class SocketChannel: public Channel, public std::enable_shared_from_this<SocketChannel>
{
public:
    struct tagChannelCtx
    {
        int iFd;
        int iCodecType;
    };

    SocketChannel(std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, bool bWithSsl = false, ev_tstamp dKeepAlive = 0.0);
    virtual ~SocketChannel();
    
    static int SendChannelFd(int iSocketFd, int iSendFd, int iCodecType, std::shared_ptr<NetLogger> pLogger);
    static int RecvChannelFd(int iSocketFd, int& iRecvFd, int& iCodecType, std::shared_ptr<NetLogger> pLogger);


    const std::string& GetIdentify() const;
    const std::string& GetRemoteAddr() const;
    const std::string& GetClientData() const;

private:
    //std::unique_ptr<SocketChannelImpl> m_pImpl;
    std::shared_ptr<SocketChannelImpl> m_pImpl;
    
    friend class WorkerImpl;
    friend class Manager;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_SOCKETCHANNEL_HPP_ */

