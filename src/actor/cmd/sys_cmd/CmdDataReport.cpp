/*******************************************************************************
 * Project:  Nebula
 * @file     CmdDataReport.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdDataReport.hpp"
#include "pb/report.pb.h"
#include "actor/session/sys_session/SessionDataReport.hpp"

namespace neb
{

CmdDataReport::CmdDataReport(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdDataReport::~CmdDataReport()
{
}

bool CmdDataReport::Init()
{
    if (pSessionDataReport == nullptr)
    {
        std::string strReportSessionId = "neb::SessionDataReport";
        auto pSharedSession = MakeSharedSession("neb::SessionReport",
                strReportSessionId, GetDataReportInterval());
        if (pSharedSession == nullptr)
        {
            return(false);
        }
        pSessionDataReport = std::dynamic_pointer_cast<SessionDataReport>(pSharedSession);
    }
    return(true);
}

bool CmdDataReport::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    LOG4_TRACE("");
    std::shared_ptr<Report> pReport = std::make_shared<Report>();
    if (pReport == nullptr)
    {
        LOG4_ERROR("failed to new Report!");
        return(CMD_STATUS_FAULT);
    }
    if (!pReport->ParseFromString(oInMsgBody.data()))
    {
        LOG4_ERROR("Report.ParseFromString() failed!");
        return(false);
    }
    if (pSessionDataReport == nullptr)
    {
        std::string strReportSessionId = "neb::SessionDataReport";
        auto pSharedSession = MakeSharedSession("neb::SessionDataReport",
                strReportSessionId, GetDataReportInterval());
        if (pSharedSession == nullptr)
        {
            return(false);
        }
        pSessionDataReport = std::dynamic_pointer_cast<SessionDataReport>(pSharedSession);
    }
    LOG4_TRACE("data report from %s", pChanel->GetIdentify().c_str());
    pSessionDataReport->AddReport(pReport);
    if (GetNodeType() == "BEACON")
    {
        pSessionDataReport->AddReport(pChannel->GetIdentify(), pReport);
    }
    return(true);
}

} /* namespace neb */
