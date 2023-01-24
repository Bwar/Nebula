/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnWorkerLoad.hpp
 * @brief    接收Worker节点负载等信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnWorkerLoad.hpp"
#include "pb/report.pb.h"
#include "channel/SocketChannel.hpp"
#include "channel/SpecChannel.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnWorkerLoad::CmdOnWorkerLoad(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnWorkerLoad::~CmdOnWorkerLoad()
{
}

bool CmdOnWorkerLoad::AnyMessage(
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
    ReportRecord oWorkerStatus;
    if (oWorkerStatus.ParseFromString(oInMsgBody.data()))
    {
        auto pSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
        return(m_pSessionManager->SetWorkerLoad(pSpecChannel->GetFromLaborId(), oWorkerStatus));
    }
    else
    {
        LOG4_ERROR("oInMsgBody.data() is not a json!");
        return(false);
    }
}

} /* namespace neb */
