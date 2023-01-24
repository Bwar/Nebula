/*******************************************************************************
 * Project:  Nebula
 * @file     CmdFdTransfer.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-08
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CmdFdTransfer.hpp"
#include "pb/neb_sys.pb.h"
#include "channel/SocketChannel.hpp"
#include "ios/Dispatcher.hpp"
#include "labor/NodeInfo.hpp"
#include "actor/step/PbStep.hpp"

namespace neb
{

CmdFdTransfer::CmdFdTransfer(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdFdTransfer::~CmdFdTransfer()
{
}

bool CmdFdTransfer::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    FdTransfer oFdTransferInfo;
    if (!oFdTransferInfo.ParseFromString(oInMsgBody.data()))
    {
        LOG4_ERROR("SpecChannelInfo ParseFromString failed.");
        return(false);
    }
    LOG4_INFO("%s:%d fd[%d] transfer, addr_family %d with codec_type %d",
            oFdTransferInfo.client_addr().c_str(),
            oFdTransferInfo.client_port(), oFdTransferInfo.fd(),
            oFdTransferInfo.addr_family(), oFdTransferInfo.codec_type());
    std::shared_ptr<SocketChannel> pNewChannel = nullptr;
    if ((CODEC_NEBULA != oFdTransferInfo.codec_type()) && (CODEC_NEBULA_IN_NODE != oFdTransferInfo.codec_type()) && GetLabor(this)->WithSsl())
    {
        pNewChannel = GetLabor(this)->GetDispatcher()->CreateSocketChannel(
                oFdTransferInfo.fd(), E_CODEC_TYPE(oFdTransferInfo.codec_type()), false, true);
    }
    else
    {
        pNewChannel = GetLabor(this)->GetDispatcher()->CreateSocketChannel(
                oFdTransferInfo.fd(), E_CODEC_TYPE(oFdTransferInfo.codec_type()), false, false);
    }
    if (nullptr != pNewChannel)
    {
        pNewChannel->SetRemoteAddr(oFdTransferInfo.client_addr());
        GetLabor(this)->GetDispatcher()->AddIoReadEvent(pNewChannel);
        if (CODEC_NEBULA == oFdTransferInfo.codec_type())
        {
            GetLabor(this)->GetDispatcher()->AddIoTimeout(pNewChannel, GetNodeInfo().dIoTimeout);
            std::shared_ptr<Step> pStepTellWorker
                = GetLabor(this)->GetActorBuilder()->MakeSharedStep(nullptr, "neb::StepTellWorker", pNewChannel);
            if (nullptr == pStepTellWorker)
            {
                return(false);
            }
            pStepTellWorker->Emit(ERR_OK);
        }
        else
        {
            pNewChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            ev_tstamp dIoTimeout = (GetNodeInfo().dConnectionProtection > 0)
                    ? GetNodeInfo().dConnectionProtection : GetNodeInfo().dIoTimeout;
            GetLabor(this)->GetDispatcher()->AddIoTimeout(pNewChannel, dIoTimeout);
        }
        GetLabor(this)->IoStatAddConnection(IO_STAT_DOWNSTREAM_NEW_CONNECTION);
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(oFdTransferInfo.fd());
    }
    return(true);
}

} /* namespace neb */

