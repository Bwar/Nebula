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
    : m_bIsClient(false), m_bWithSsl(false), m_pImpl(nullptr), m_pLogger(nullptr), m_pWatcher(nullptr)
{
}

SocketChannel::SocketChannel(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, bool bWithSsl, bool bIsClient, ev_tstamp dKeepAlive)
    : m_bIsClient(bIsClient), m_bWithSsl(bWithSsl), m_pImpl(nullptr), m_pLogger(nullptr), m_pWatcher(nullptr)
{
    if (bWithSsl)
    {
#ifdef WITH_OPENSSL
        pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "with openssl, create SocekChannelSslImpl.");
        auto pImpl = std::make_shared<SocketChannelSslImpl<CodecNebula>>(pLabor, pLogger, iFd, ulSeq, dKeepAlive);
        pImpl->Init(bIsClient);
        m_pImpl = std::static_pointer_cast<SocketChannel>(pImpl);
#else
        pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "without openssl, SocekChannelImpl instead.");
        auto pImpl = std::make_shared<SocketChannelImpl<CodecNebula>>(pLabor, pLogger, iFd, ulSeq, dKeepAlive);
        m_pImpl = std::static_pointer_cast<SocketChannel>(pImpl);
#endif
    }
    else
    {
        pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "create SocekChannelImpl.");
        auto pImpl = std::make_shared<SocketChannelImpl<CodecNebula>>(pLabor, pLogger, iFd, ulSeq, dKeepAlive);
        m_pImpl = std::static_pointer_cast<SocketChannel>(pImpl);
    }
}

SocketChannel::~SocketChannel()
{
    if (m_pLogger != nullptr)
    {
        m_pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "");
    }
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
        return(-1);
    }
    return(m_pImpl->GetFd());
}

uint32 SocketChannel::GetSequence() const
{
    if (m_pImpl == nullptr)
    {
        return(0);
    }
    return(m_pImpl->GetSequence());
}

bool SocketChannel::IsPipeline() const
{
    if (m_pImpl == nullptr)
    {
        return(false);
    }
    return(m_pImpl->IsPipeline());
}

const std::string& SocketChannel::GetIdentify() const
{
    if (m_pImpl == nullptr)
    {
        return(m_strEmpty);
    }
    return(m_pImpl->GetIdentify());
}

const std::string& SocketChannel::GetRemoteAddr() const
{
    if (m_pImpl == nullptr)
    {
        return(m_strEmpty);
    }
    return(m_pImpl->GetRemoteAddr());
}

const std::string& SocketChannel::GetClientData() const
{
    if (m_pImpl == nullptr)
    {
        return(m_strEmpty);
    }
    return(m_pImpl->GetClientData());
}

E_CODEC_TYPE SocketChannel::GetCodecType() const
{
    if (m_pImpl == nullptr)
    {
        return(CODEC_UNKNOW);
    }
    return(m_pImpl->GetCodecType());
}

uint8 SocketChannel::GetChannelStatus() const
{
    if (m_pImpl == nullptr)
    {
        return(0);
    }
    return(m_pImpl->GetChannelStatus());
}

uint32 SocketChannel::PopStepSeq(uint32 uiStreamId, E_CODEC_STATUS eStatus)
{
    if (m_pImpl == nullptr)
    {
        return(0);
    }
    return(m_pImpl->PopStepSeq(uiStreamId, eStatus));
}

bool SocketChannel::PipelineIsEmpty() const
{
    if (m_pImpl == nullptr)
    {
        return(0);
    }
    return(m_pImpl->PipelineIsEmpty());
}

int SocketChannel::GetErrno() const
{
    if (m_pImpl == nullptr)
    {
        return(0);
    }
    return(m_pImpl->GetErrno());
}

const std::string& SocketChannel::GetErrMsg() const
{
    if (m_pImpl == nullptr)
    {
        return(m_strEmpty);
    }
    return(m_pImpl->GetErrMsg());
}

Codec* SocketChannel::GetCodec() const
{
    if (m_pImpl == nullptr)
    {
        return(nullptr);
    }
    return(m_pImpl->GetCodec());
}

bool SocketChannel::InitImpl(std::shared_ptr<SocketChannel> pImpl)
{
    if (pImpl == nullptr)
    {
        return(false);
    }
    m_pImpl = pImpl;
    return(true);
}

Labor* SocketChannel::GetLabor()
{
    return(nullptr);
}

int SocketChannel::SendChannelFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType, std::shared_ptr<NetLogger> pLogger)
{
    ssize_t             n;
    struct iovec        iov[1];
    struct msghdr       msg;
    tagChannelCtx stCh;
    int iError = 0;

    stCh.iFd = iSendFd;
    stCh.iAiFamily = iAiFamily;
    stCh.iCodecType = iCodecType;

    union
    {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    if (stCh.iFd == -1)
    {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;

    }
    else
    {
        msg.msg_control = (caddr_t) &cmsg;
        msg.msg_controllen = sizeof(cmsg);

        memset(&cmsg, 0, sizeof(cmsg));

        cmsg.cm.cmsg_len = CMSG_LEN(sizeof(int));
        cmsg.cm.cmsg_level = SOL_SOCKET;
        cmsg.cm.cmsg_type = SCM_RIGHTS;

        *(int *) CMSG_DATA(&cmsg.cm) = stCh.iFd;
    }

    msg.msg_flags = 0;

    iov[0].iov_base = (char*)&stCh;
    iov[0].iov_len = sizeof(tagChannelCtx);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    n = sendmsg(iSocketFd, &msg, 0);

    if (n == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "sendmsg() failed, errno %d", errno);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    return(ERR_OK);
}

int SocketChannel::RecvChannelFd(int iSocketFd, int& iRecvFd, int& iAiFamily, int& iCodecType, std::shared_ptr<NetLogger> pLogger)
{
    ssize_t             n;
    struct iovec        iov[1];
    struct msghdr       msg;
    tagChannelCtx stCh;
    int iError = 0;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    iov[0].iov_base = (char*)&stCh;
    iov[0].iov_len = sizeof(tagChannelCtx);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);

    n = recvmsg(iSocketFd, &msg, 0);

    if (n == -1) {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "recvmsg() failed, errno %d", errno);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    if (n == 0) {
        pLogger->WriteLog(neb::Logger::WARNING, __FILE__, __LINE__, __FUNCTION__, "recvmsg() return zero, errno %d", errno);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(ERR_CHANNEL_EOF);
    }

    if ((size_t) n < sizeof(tagChannelCtx))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "rrecvmsg() returned not enough data: %z, errno %d", n, errno);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int)))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "recvmsg() returned too small ancillary data");
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__,
                        "recvmsg() returned invalid ancillary data level %d or type %d", cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    stCh.iFd = *(int *) CMSG_DATA(&cmsg.cm);

    if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC))
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "recvmsg() truncated data");
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    iRecvFd = stCh.iFd;
    iAiFamily = stCh.iAiFamily;
    iCodecType = stCh.iCodecType;

    return(ERR_OK);
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
            m_pLogger->WriteLog(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "new ChannelWatcher error %s", e.what());
        }
    }
    return(m_pWatcher);
}

}
