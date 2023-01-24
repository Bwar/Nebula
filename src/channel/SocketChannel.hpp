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
class SocketChannelMigrate;
class CmdChannelMigrate;
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
    SocketChannel(std::shared_ptr<NetLogger> pLogger, bool bIsClient, bool bWithSsl);
    virtual ~SocketChannel();
    
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
    virtual ev_tstamp GetActiveTime() const;
    virtual ev_tstamp GetPenultimateActiveTime() const;
    virtual ev_tstamp GetLastRecvTime() const;
    virtual ev_tstamp GetKeepAlive() const;
    virtual int GetErrno() const;
    virtual const std::string& GetErrMsg() const;
    virtual Codec* GetCodec() const;
    virtual bool IsChannelVerify() const;
    virtual bool NeedAliveCheck() const;
    virtual uint32 GetMsgNum() const;
    virtual uint32 GetUnitTimeMsgNum() const;
    virtual E_CODEC_STATUS Send();
    virtual uint32 GetPeerStepSeq() const;
    virtual void SetChannelStatus(E_CHANNEL_STATUS eStatus);
    virtual void SetClientData(const std::string& strClientData);
    virtual void SetIdentify(const std::string& strIdentify);
    virtual void SetRemoteAddr(const std::string& strRemoteAddr);

    bool IsMigrated() const
    {
        return(m_bMigrated);
    }
    ChannelWatcher* MutableWatcher();

    template <typename ...Targs>
    void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args) const;
protected:
    virtual Labor* GetLabor();
    virtual int16 GetRemoteWorkerIndex() const;
    virtual bool Close();
    virtual void SetBonding(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, std::shared_ptr<SocketChannel> pBindChannel);
    void SetMigrated(bool bMigrated);
    bool InitImpl(std::shared_ptr<SocketChannel> pImpl);

private:
    bool m_bIsClient;
    bool m_bWithSsl;
    bool m_bMigrated;
    std::string m_strEmpty;
    // Hide most of the channel implementation for Actors
    std::shared_ptr<SocketChannel> m_pImpl;
    std::shared_ptr<NetLogger> m_pLogger;
    ChannelWatcher* m_pWatcher;
    
    friend class SocketChannelMigrate;
    friend class CmdChannelMigrate;
    friend class Dispatcher;
    friend class ActorBuilder;
    friend class CodecFactory;
    friend class CodecProto;
    template<typename T> friend class IO;
    template<typename T> friend class SocketChannelImpl;
};

template <typename ...Targs>
void SocketChannel::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args) const
{
    if (m_pLogger != nullptr)
    {
        m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
    }
}

} /* namespace neb */

#endif /* SRC_CHANNEL_SOCKETCHANNEL_HPP_ */

