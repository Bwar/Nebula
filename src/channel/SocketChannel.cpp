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

SocketChannel::SocketChannel(std::shared_ptr<NetLogger> pLogger, bool bIsClient, bool bWithSsl)
    : m_bIsClient(bIsClient), m_bWithSsl(bWithSsl), m_pImpl(nullptr), m_pLogger(pLogger), m_pWatcher(nullptr)
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

bool SocketChannel::Close()
{
    if (m_pImpl == nullptr)
    {
        LOG4_TRACE("m_pImpl is nullptr");
        return(false);
    }
    return(m_pImpl->Close());
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

int SocketChannel::SendChannelFd(int iSocketFd, int iSendFd, int iAiFamily, int iCodecType,
        const std::string& strRemoteAddr, std::shared_ptr<NetLogger> pLogger)
{
    ssize_t             n;
    struct iovec        iov[2];
    struct msghdr       msg;
    tagChannelCtx stCh;
    int iError = 0;
    char szAddress[64] = {0};

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

    snprintf(szAddress, 64, "%s", strRemoteAddr.data());
    iov[0].iov_base = (void*)&stCh;
    iov[0].iov_len = sizeof(tagChannelCtx);
    iov[1].iov_base = (void*)szAddress;
    iov[1].iov_len = 64;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    n = sendmsg(iSocketFd, &msg, 0);

    if (n == -1)
    {
        pLogger->WriteLog(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, "sendmsg() failed, errno %d", errno);
        iError = (errno == 0) ? ERR_TRANSFER_FD : errno;
        return(iError);
    }

    return(ERR_OK);
}

int SocketChannel::RecvChannelFd(int iSocketFd, int& iRecvFd, int& iAiFamily, int& iCodecType,
        std::string& strRemoteAddr, std::shared_ptr<NetLogger> pLogger)
{
    ssize_t             n;
    struct iovec        iov[2];
    struct msghdr       msg;
    tagChannelCtx stCh;
    int iError = 0;
    char szAddress[64] = {0};

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    iov[0].iov_base = (void*)&stCh;
    iov[0].iov_len = sizeof(tagChannelCtx);
    iov[1].iov_base = (void*)szAddress;
    iov[1].iov_len = 64;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

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
    strRemoteAddr = szAddress;

    return(ERR_OK);
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
