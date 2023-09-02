/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannel.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include "SocketChannel.hpp"
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "ios/ChannelWatcher.hpp"
#include "SocketChannelImpl.hpp"
#ifdef WITH_OPENSSL
#include "SocketChannelSslImpl.hpp"
#endif
#include "codec/CodecProto.hpp"

namespace neb
{

SocketChannel::SocketChannel()
    : m_bIsClient(false), m_bWithSsl(false), m_bMigrated(false), m_pImpl(nullptr), m_pLogger(nullptr), m_pWatcher(nullptr)
{
}

SocketChannel::SocketChannel(std::shared_ptr<NetLogger> pLogger, bool bIsClient, bool bWithSsl)
    : m_bIsClient(bIsClient), m_bWithSsl(bWithSsl), m_bMigrated(false), m_pImpl(nullptr), m_pLogger(pLogger), m_pWatcher(nullptr)
{
}

SocketChannel::~SocketChannel()
{
    LOG4_TRACE("");
    if (nullptr != m_pWatcher)
    {
        delete m_pWatcher;
        m_pWatcher = nullptr;
    }
}

bool SocketChannel::IsClient() const
{
    return(m_bIsClient);
}

bool SocketChannel::WithSsl() const
{
    return(m_bWithSsl);
}
int SocketChannel::GetFd() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(-1);
    }
    return(m_pImpl->GetFd());
}

uint32 SocketChannel::GetSequence() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0);
    }
    return(m_pImpl->GetSequence());
}

bool SocketChannel::IsPipeline() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->IsPipeline());
}

const std::string& SocketChannel::GetIdentify() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(m_strEmpty);
    }
    return(m_pImpl->GetIdentify());
}

const std::string& SocketChannel::GetRemoteAddr() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(m_strEmpty);
    }
    return(m_pImpl->GetRemoteAddr());
}

const std::string& SocketChannel::GetClientData() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(m_strEmpty);
    }
    return(m_pImpl->GetClientData());
}

E_CODEC_TYPE SocketChannel::GetCodecType() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(CODEC_UNKNOW);
    }
    return(m_pImpl->GetCodecType());
}

uint8 SocketChannel::GetChannelStatus() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0);
    }
    return(m_pImpl->GetChannelStatus());
}

uint32 SocketChannel::PopStepSeq(uint32 uiStreamId, E_CODEC_STATUS eStatus)
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0);
    }
    return(m_pImpl->PopStepSeq(uiStreamId, eStatus));
}

bool SocketChannel::PipelineIsEmpty() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0);
    }
    return(m_pImpl->PipelineIsEmpty());
}

ev_tstamp SocketChannel::GetActiveTime() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0.0);
    }
    return(m_pImpl->GetActiveTime());
}

ev_tstamp SocketChannel::GetPenultimateActiveTime() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0.0);
    }
    return(m_pImpl->GetPenultimateActiveTime());
}

ev_tstamp SocketChannel::GetLastRecvTime() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0.0);
    }
    return(m_pImpl->GetLastRecvTime());
}

ev_tstamp SocketChannel::GetKeepAlive() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0.0);
    }
    return(m_pImpl->GetKeepAlive());
}

int SocketChannel::GetErrno() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(0);
    }
    return(m_pImpl->GetErrno());
}

const std::string& SocketChannel::GetErrMsg() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(m_strEmpty);
    }
    return(m_pImpl->GetErrMsg());
}

Codec* SocketChannel::GetCodec() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(nullptr);
    }
    return(m_pImpl->GetCodec());
}

bool SocketChannel::IsChannelVerify() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->IsChannelVerify());
}

bool SocketChannel::NeedAliveCheck() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->NeedAliveCheck());
}

uint32 SocketChannel::GetMsgNum() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->GetMsgNum());
}

uint32 SocketChannel::GetUnitTimeMsgNum() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->GetUnitTimeMsgNum());
}

E_CODEC_STATUS SocketChannel::Send()
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(CODEC_STATUS_ERR);
    }
    return(m_pImpl->Send());
}

int16 SocketChannel::GetRemoteWorkerIndex() const
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->GetRemoteWorkerIndex());
}

void SocketChannel::SetChannelStatus(E_CHANNEL_STATUS eStatus)
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
    }
    return(m_pImpl->SetChannelStatus(eStatus));
}

void SocketChannel::SetClientData(const std::string& strClientData)
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
    }
    return(m_pImpl->SetClientData(strClientData));
}

void SocketChannel::SetIdentify(const std::string& strIdentify)
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
    }
    return(m_pImpl->SetIdentify(strIdentify));
}

void SocketChannel::SetRemoteAddr(const std::string& strRemoteAddr)
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
    }
    return(m_pImpl->SetRemoteAddr(strRemoteAddr));
}

void SocketChannel::SetKeepAlive(ev_tstamp dTime)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->SetKeepAlive(dTime);
    }
}

bool SocketChannel::Close()
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->Close());
}

void SocketChannel::SetBonding(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, std::shared_ptr<SocketChannel> pBindChannel)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->SetBonding(pLabor, pLogger, pBindChannel);
    }
    m_pLogger = pLogger;
}

void SocketChannel::SetMigrated(bool bMigrated)
{
    m_bMigrated = bMigrated;
}

bool SocketChannel::InitImpl(std::shared_ptr<SocketChannel> pImpl)
{
    if (pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    m_pImpl = pImpl;
    return(true);
}

Labor* SocketChannel::GetLabor()
{
    return(nullptr);
}

uint32 SocketChannel::GetPeerStepSeq() const
{
    return(0);
}

ChannelWatcher* SocketChannel::MutableWatcher()
{
    if (nullptr == m_pWatcher)
    {
        try
        {
            m_pWatcher = new ChannelWatcher();
        }
        catch(std::bad_alloc& e)
        {
            LOG4_TRACE("new ChannelWatcher error %s", e.what());
        }
    }
    return(m_pWatcher);
}

}
