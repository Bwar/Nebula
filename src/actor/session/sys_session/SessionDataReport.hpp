/*******************************************************************************
 * Project:  Nebula
 * @file     SessionDataReport.hpp
 * @brief    数据上报
 * @author   Bwar
 * @date:    2019-12-22
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef ACTOR_SESSION_SYS_SESSION_SESSIONDATAREPORT_HPP_
#define ACTOR_SESSION_SYS_SESSION_SESSIONDATAREPORT_HPP_

#include <string>
#include <unordered_map>
#include "actor/session/Timer.hpp"
#include "actor/ActorSys.hpp"
#include "pb/report.pb.h"

namespace neb
{

class SessionDataReport: public Timer,
    DynamicCreator<SessionDataReport, std::string&, ev_tstamp>,
    public ActorSys
{
public:
    SessionDataReport(const std::string& strSessionId, ev_tstamp dStatInterval = gc_dDefaultTimeout);
    virtual ~SessionDataReport();

    virtual E_CMD_STATUS Timeout();

    void AddReport(const std::shared_ptr<Report> pReport);
    void AddReport(const std::string& strNodeIdentify, std::shared_ptr<Report> pReport);

    const Report* GetReport()
    {
        return(m_pReport);
    }

    const std::string& GetReportString()
    {
        return(m_strReport);
    }

    const std::unordered_map<std::string, std::shared_ptr<Report>>& GetNodeReport() const
    {
        return(m_vecNodeReport[1 - m_uiNodeReportUpdatingIndex]);
    }
                    
private:
    uint32 m_uiNodeReportUpdatingIndex;
    Report* m_pReport;          ///< final report
    std::string m_strReport;    ///< m_pReport SerializeToString
    std::unordered_map<std::string, ReportRecord*> m_mapDataCollect;   ///< report collector
    std::vector<std::unordered_map<std::string, std::shared_ptr<Report>> > m_vecNodeReport;  ///< report data from nebula node
};

}

#endif /* ACTOR_SESSION_SYS_SESSION_SESSIONDATAREPORT_HPP_ */
