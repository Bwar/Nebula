/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannel.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "SocketChannel.hpp"

namespace neb
{

SocketChannel::SocketChannel(std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, bool bWithSsl, ev_tstamp dKeepAlive)
    : m_pImpl(nullptr), m_pLogger(pLogger)
{
    if (bWithSsl)
    {
#ifdef WITH_OPENSSL
        pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "with openssl, create SocekChannelSslImpl.");
        m_pImpl = std::dynamic_pointer_cast<SocketChannelImpl>(std::make_shared<SocketChannelSslImpl>(this, pLogger, iFd, ulSeq, dKeepAlive));
#else
        pLogger->WriteLog(Logger::INFO, __FILE__, __LINE__, __FUNCTION__, "without openssl, SocekChannelImpl instead.");
        m_pImpl = std::make_shared<SocketChannelImpl>(this, pLogger, iFd, ulSeq, dKeepAlive);
#endif
    }
    else
    {
        pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "create SocekChannelImpl.");
        m_pImpl = std::make_shared<SocketChannelImpl>(this, pLogger, iFd, ulSeq, dKeepAlive);
    }
}

SocketChannel::~SocketChannel()
{
    m_pLogger->WriteLog(Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, "");
}

int SocketChannel::GetFd() const
{
    return(m_pImpl->GetFd());
}

const std::string& SocketChannel::GetIdentify() const
{
    return(m_pImpl->GetIdentify());
}

const std::string& SocketChannel::GetRemoteAddr() const
{
    return(m_pImpl->GetRemoteAddr());
}

const std::string& SocketChannel::GetClientData() const
{
    return(m_pImpl->GetClientData());
}

E_CODEC_TYPE SocketChannel::GetCodecType() const
{
    return(m_pImpl->GetCodecType());
}

uint32 SocketChannel::GetStepSeq() const
{
    return(m_pImpl->GetStepSeq());
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

}
