/*******************************************************************************
 * Project:  Nebula
 * @file     SessionDataReport.cpp
 * @brief    数据上报
 * @author   Bwar
 * @date:    2019-12-22
 * @note
 * Modify history:
 ******************************************************************************/

#include "SessionDataReport.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

SessionDataReport::SessionDataReport(const std::string& strSessionId, ev_tstamp dStatInterval)
    : Timer(strSessionId, dStatInterval), m_pReport(nullptr)
{
}

SessionDataReport::~SessionDataReport()
{
    if (m_pReport != nullptr)
    {
        delete m_pReport;
        m_pReport = nullptr;
    }
}

E_CMD_STATUS SessionDataReport::Timeout()
{
    if (m_mapData.empty())
    {
        return(CMD_STATUS_RUNNING);
    }
    if (m_pReport != nullptr)
    {
        delete m_pReport;
        m_pReport = nullptr;
    }
    try
    {
        m_pReport = new Report();
    }
    catch (std::bad_alloc& e)
    {
        LOG4_ERROR("failed to new Report!");
        return(CMD_STATUS_FAULT);
    }
    for (auto iter = m_mapData.begin(); iter != m_mapData.end(); ++iter)
    {
        m_pReport->mutable_records()->AddAllocated(iter->second);
        iter->second = nullptr;
    }
    if (Labor::LABOR_MANAGER == GetLabor(this)->GetLaborType())
    {
        MsgBody oMsgBody;
        m_pReport->SerializeToString(&m_strReport);
        oMsgBody.set_data(m_strReport);
        SendDataReport(CMD_REQ_DATA_REPORT, GetSequence(), oMsgBody);
    }
    m_mapData.clear();
    return(CMD_STATUS_RUNNING);
}

void SessionDataReport::AddReport(const Report& oReport)
{
    std::unordered_map<std::string, ReportRecord*>::iterator iter;
    for (int i = 0; i < oReport.records_size(); ++i)
    {
        iter = m_mapData.find(oReport.records(i).key());
        if (iter == m_mapData.end())
        {
            ReportRecord* pRecord = nullptr;
            try
            {
                pRecord = new ReportRecord();
            }
            catch (std::bad_alloc& e)
            {
                return;
            }
            pRecord->set_key(oReport.records(i).key());
            for (int j = 0; j < oReport.records(i).value_size(); ++j)
            {
                pRecord->add_value(oReport.records(i).value(j));
            }
            m_mapData.insert(std::make_pair(oReport.records(i).key(), pRecord));
        }
        else
        {
            for (int j = 0; j < oReport.records(i).value_size(); ++j)
            {
                if (j < iter->second->value_size())
                {
                    iter->second->set_value(j, iter->second->value(j) + oReport.records(i).value(j));
                }
                else
                {
                    iter->second->add_value(oReport.records(i).value(j));
                }
            }
        }
    }
}

}

