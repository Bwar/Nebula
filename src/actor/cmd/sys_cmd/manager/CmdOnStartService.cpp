/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnStartService.hpp
 * @brief    
 * @author   Bwar
 * @date:    2020-07-05
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnStartService.hpp"
#include "labor/NodeInfo.hpp"
#include "labor/Manager.hpp"

namespace neb
{

CmdOnStartService::CmdOnStartService(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnStartService::~CmdOnStartService()
{
}

bool CmdOnStartService::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    Labor* pLabor = GetLabor(this);
    if (pLabor->GetNodeInfo().uiWorkerNum + pLabor->GetNodeInfo().uiLoaderNum
            == m_setReadyWorker.size())
    {
        return(true); // worker restarted
    }

    uint32 uiWorkerId = strtoul(oInMsgBody.data().c_str(), NULL, 10);
    m_setReadyWorker.insert(uiWorkerId);
    if (pLabor->GetNodeInfo().uiWorkerNum + pLabor->GetNodeInfo().uiLoaderNum
            == m_setReadyWorker.size())
    {
        ((Manager*)pLabor)->StartService();
    }
    else
    {
        LOG4_TRACE("waitting for worker or loader ready...");
    }
    return(true);
}

} /* namespace neb */
