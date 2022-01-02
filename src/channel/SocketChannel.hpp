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

#include <string>
#include <memory>
#include "Channel.hpp"
#include "codec/Codec.hpp"

namespace neb
{

class Dispatcher;
class ChannelWatcher;
class Labor;
class CodecFactory;
class CodecProto;

template<typename T> class IO;
template<typename T> class SocketChannelImpl;

class SocketChannel: public Channel
{
public:
    struct tagChannelCtx
    {
        int iFd;
        int iAiFamily;        // AF_INET  or   AF_INET6
        int iCodecType;
    };

    SocketChannel(); // only for SelfChannel
    SocketChannel(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, bool bWithSsl, bool bIsClient, ev_tstamp dKeepAlive = 10.0);
    virtual ~SocketChannel();
    
    static int SendChannelFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType, std::shared_ptr<NetLogger> pLogger);
    static int RecvChannelFd(int iSocketFd, int& iRecvFd, int& iAiFamily, int& iCodecType, std::shared_ptr<NetLogger> pLogger);

    virtual bool IsClient() const;
    virtual bool WithSsl() const;
    virtual int GetFd() const;
    virtual uint32 GetSequence() const;
    virtual bool IsPipeline() const;
    virtual const std::string& GetIdentify() const;
    virtual const std::string& GetRemoteAddr() const;
    virtual const std::string& GetClientData() const;
    virtual E_CODEC_TYPE GetCodecType() const;
    virtual uint8 GetChannelStatus() const;
    virtual uint32 PopStepSeq(uint32 uiStreamId = 0, E_CODEC_STATUS eStatus = CODEC_STATUS_OK);
    virtual bool PipelineIsEmpty() const;
    virtual int GetErrno() const;
    virtual const std::string& GetErrMsg() const;
    virtual Codec* GetCodec() const;
    ChannelWatcher* MutableWatcher();

protected:
    virtual Labor* GetLabor();
    bool InitImpl(std::shared_ptr<SocketChannel> pImpl);

private:
    bool m_bIsClient;
    bool m_bWithSsl;
    std::string m_strEmpty;
    // Hide most of the channel implementation for Actors
    std::shared_ptr<SocketChannel> m_pImpl;
    std::shared_ptr<NetLogger> m_pLogger;
    ChannelWatcher* m_pWatcher;
    
    friend class Dispatcher;
    friend class ActorBuilder;
    friend class CodecFactory;
    friend class CodecProto;
    template<typename T> friend class IO;
    template<typename T> friend class SocketChannelImpl;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_SOCKETCHANNEL_HPP_ */

