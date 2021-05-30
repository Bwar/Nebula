/*******************************************************************************
 * Project:  Nebula
 * @file     ModuleMetrics.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-05-30
 * @note
 * Modify history:
 ******************************************************************************/

#include "ModuleMetrics.hpp"
#include <sstream>
#include "actor/session/sys_session/SessionDataReport.hpp"

namespace neb
{

ModuleMetrics::ModuleMetrics(const std::string& strModulePath)
    : Module(strModulePath)
{
}

ModuleMetrics::~ModuleMetrics()
{
}

bool ModuleMetrics::Init()
{
    m_strApp = GetCustomConf()("app");
    return(true);
}

bool ModuleMetrics::AnyMessage(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(200);
    oOutHttpMsg.set_http_major(oHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
    oOutHttpMsg.mutable_headers()->insert({"Content-Type", "text/plain;charset=UTF-8"});

    std::string strSessionId = "neb::SessionDataReport";
    auto pSharedSession = GetSession(strSessionId);
    if (pSharedSession == nullptr)
    {
        LOG4_ERROR("no session named \"neb::SessionDataReport\"!");
        SendTo(pChannel, oOutHttpMsg);
        return(false);
    }
    auto pReportSession = std::dynamic_pointer_cast<SessionDataReport>(pSharedSession);

    std::ostringstream oss;
    auto pReport = pReportSession->GetReport();
    if (pReport == nullptr)
    {
        SendTo(pChannel, oOutHttpMsg);
        return(false);
    }
    for (int i = 0; i < pReport->records_size(); ++i)
    {
        if (pReport->records(i).item() == "nebula")
        {
            if (pReport->records(i).value_size() > 0)
            {
                if (m_strApp.empty())
                {
                    oss << "nebula{key=\"}" << pReport->records(i).key()
                        << "\"}" << pReport->records(i).value(0) << "\n";
                }
                else
                {
                    oss << "nebula{app=\"" << m_strApp
                        << "\", key=\"}" << pReport->records(i).key()
                        << "\"}" << pReport->records(i).value(0) << "\n";
                }
            }
            else
            {
                LOG4_TRACE("report item \"%s\" not define.", pReport->records(i).item().c_str());
            }
        }
    }
    oOutHttpMsg.set_body(oss.str());
    SendTo(pChannel, oOutHttpMsg);
    return(true);
}

}
