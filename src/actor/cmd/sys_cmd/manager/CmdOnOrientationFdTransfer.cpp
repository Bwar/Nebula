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
#include "codec/CodecProto.hpp"

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
    uint32 uiManagerLaborId = GetLabor(this)->GetNodeInfo().uiWorkerNum + 1;
    bool bResult = GetLabor(this)->GetDispatcher()->MigrateSocketChannel(uiManagerLaborId, iWorkerIndex, pChannel);
    if (bResult)
    {
        return(true);
    }
    else
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_TRANSFER_FD);
        oOutMsgBody.mutable_rsp_result()->set_msg("channel migration error");
        SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
        return(false);
    }
}

} /* namespace neb */
