/*******************************************************************************
 * Project:  Nebula
 * @file     StepReportToBeacon.cpp
 * @brief    向注册中心上报
 * @author   Bwar
 * @date:    2019年9月22日
 * @note
 * Modify history:
 ******************************************************************************/

#include "StepReportToBeacon.hpp"
#include "labor/NodeInfo.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

StepReportToBeacon::StepReportToBeacon(ev_tstamp dTimeout)
    : PbStep(nullptr, dTimeout)
{
}

StepReportToBeacon::~StepReportToBeacon()
{
}

E_CMD_STATUS StepReportToBeacon::Emit(
        int iErrno, const std::string& strErrMsg, void* data)
{
    if (std::string("BEACON") == GetLabor(this)->GetNodeInfo().strNodeType)
    {
        return(CMD_STATUS_RUNNING);
    }
    MsgBody oMsgBody;
    CJsonObject oReportData;
    if (m_pSessionManager == nullptr)
    {
        m_pSessionManager = std::dynamic_pointer_cast<SessionManager>(
                GetSession("neb::SessionManager"));
        if (m_pSessionManager == nullptr)
        {
            LOG4_ERROR("no session named \"neb::SessionManager\"!");
            return(CMD_STATUS_FAULT);
        }
    }
    m_pSessionManager->MakeReportData(oReportData);
    oMsgBody.set_data(oReportData.ToString());
    GetLabor(this)->GetDispatcher()->Broadcast("BEACON",
            (int32)CMD_REQ_NODE_STATUS_REPORT, GetSequence(), oMsgBody);
    return(CMD_STATUS_RUNNING);
}

E_CMD_STATUS StepReportToBeacon::Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data)
{
    if (ERR_OK == oInMsgBody.rsp_result().code())
    {
        CJsonObject oNode(oInMsgBody.data());
        uint32 uiNodeId = 0;
        oNode.Get("node_id", uiNodeId);
        if (uiNodeId != GetLabor(this)->GetNodeInfo().uiNodeId)
        {
            GetLabor(this)->SetNodeId((uiNodeId));
            m_pSessionManager->SendToChild(CMD_REQ_REFRESH_NODE_ID, oInMsgHead.seq(), oInMsgBody);
        }
        return(CMD_STATUS_RUNNING);
    }
    else
    {
        LOG4_ERROR("report to beacon error %d: %s!", oInMsgBody.rsp_result().code(),
                oInMsgBody.rsp_result().msg().c_str());
        return(CMD_STATUS_RUNNING);
    }
}

E_CMD_STATUS StepReportToBeacon::Timeout()
{
    return(Emit());
}

} /* namespace neb */
