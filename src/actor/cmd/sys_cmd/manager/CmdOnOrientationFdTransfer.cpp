/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnOrientationFdTransfer.cpp
 * @brief    定向文件描述符传送
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnOrientationFdTransfer.hpp"
#include "labor/Manager.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnOrientationFdTransfer::CmdOnOrientationFdTransfer(int iCmd)
    : Cmd(iCmd)
{
}

CmdOnOrientationFdTransfer::~CmdOnOrientationFdTransfer()
{
}

bool CmdOnOrientationFdTransfer::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    if (m_pSessionManager == nullptr)
    {
        m_pSessionManager = std::dynamic_pointer_cast<SessionManager>(
                GetSession("neb::SessionManager"));
        if (m_pSessionManager == nullptr)
        {
            LOG4_ERROR("no session named \"neb::SessionManager\"!");
            return(false);
        }
    }
    MsgBody oOutMsgBody;
    int iWorkerIndex = std::stoi(oInMsgBody.data());
    auto pWorkerInfo = m_pSessionManager->GetWorkerInfo(iWorkerIndex);
    if (pWorkerInfo == nullptr)
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_NO_SUCH_WORKER_INDEX);
        oOutMsgBody.mutable_rsp_result()->set_msg("no such worker index!");
        SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
        return(false);
    }
    std::unordered_map<int, WorkerInfo>::iterator worker_iter;
    int iErrno = GetLabor(this)->GetDispatcher()->SendFd(
            pWorkerInfo->iDataFd, pChannel->GetFd(),
            ((Manager*)GetLabor(this))->GetManagerInfo().iS2SFamily,
            (int)pChannel->GetCodecType());
    if (iErrno != ERR_OK)
    {
        GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pChannel);
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_TRANSFER_FD);
        oOutMsgBody.mutable_rsp_result()->set_msg("send fd error!");
        SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
        return(false);
    }

    GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pChannel);
    oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
    oOutMsgBody.mutable_rsp_result()->set_msg("success");
    SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
    return(true);
}

} /* namespace neb */
