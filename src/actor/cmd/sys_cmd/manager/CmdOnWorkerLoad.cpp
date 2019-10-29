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

#include "util/json/CJsonObject.hpp"
#include "channel/SocketChannel.hpp"
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
    CJsonObject oJsonLoad;
    if (oJsonLoad.Parse(oInMsgBody.data()))
    {
        return(m_pSessionManager->SetWorkerLoad(pChannel->GetFd(), oJsonLoad));
    }
    else
    {
        LOG4_ERROR("oInMsgBody.data() is not a json!");
        return(false);
    }
}

} /* namespace neb */
