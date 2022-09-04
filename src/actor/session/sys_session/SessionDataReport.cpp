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
    : Timer(strSessionId, dStatInterval), m_uiNodeReportUpdatingIndex(0), m_pReport(nullptr)
{
    std::unordered_map<std::string, std::shared_ptr<Report>> mapNodeReport1;
    std::unordered_map<std::string, std::shared_ptr<Report>> mapNodeReport2;
    m_vecNodeReport.push_back(mapNodeReport1);
    m_vecNodeReport.push_back(mapNodeReport2);
}

SessionDataReport::~SessionDataReport()
{
    if (m_pReport != nullptr)
    {
        delete m_pReport;
        m_pReport = nullptr;
    }
    for (auto data_iter = m_mapDataCollect.begin(); data_iter != m_mapDataCollect.end(); ++data_iter)
    {
        for (auto iter = data_iter->second.begin(); iter != data_iter->second.end(); ++iter)
        {
            if (iter->second != nullptr)
            {
                delete iter->second;
                iter->second = nullptr;
            }
        }
    }
    m_mapDataCollect.clear();
    m_vecNodeReport.clear();
}

E_CMD_STATUS SessionDataReport::Timeout()
{
    LOG4_TRACE("");
    if (Labor::LABOR_MANAGER != GetLabor(this)->GetLaborType())
    {
        ((Worker*)GetLabor(this))->DataReport();
    }
    if (m_mapDataCollect.empty())
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
    for (auto data_iter = m_mapDataCollect.begin(); data_iter != m_mapDataCollect.end(); ++data_iter)
    {
        for (auto iter = data_iter->second.begin(); iter != data_iter->second.end(); ++iter)
        {
            m_pReport->mutable_records()->AddAllocated(iter->second);
            iter->second = nullptr;
        }
    }
    MsgBody oMsgBody;
    m_pReport->SerializeToString(&m_strReport);
    oMsgBody.set_data(m_strReport);
    SendDataReport(CMD_REQ_DATA_REPORT, GetSequence(), oMsgBody);
    m_uiNodeReportUpdatingIndex = 1 - m_uiNodeReportUpdatingIndex;
    m_vecNodeReport[m_uiNodeReportUpdatingIndex].clear();
    m_mapDataCollect.clear();
    return(CMD_STATUS_RUNNING);
}

void SessionDataReport::AddReport(const std::shared_ptr<Report> pReport)
{
    LOG4_TRACE("");
    std::unordered_map<std::string, std::unordered_map<std::string, ReportRecord*>>::iterator data_iter;
    std::unordered_map<std::string, ReportRecord*>::iterator iter;
    for (int i = 0; i < pReport->records_size(); ++i)
    {
        data_iter = m_mapDataCollect.find(pReport->records(i).item());
        if (data_iter == m_mapDataCollect.end())
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
            pRecord->set_key(pReport->records(i).key());
            pRecord->set_item(pReport->records(i).item());
            for (int j = 0; j < pReport->records(i).value_size(); ++j)
            {
                pRecord->add_value(pReport->records(i).value(j));
            }
            std::unordered_map<std::string, ReportRecord*> mapKeyRecord;
            mapKeyRecord.insert(std::make_pair(pReport->records(i).key(), pRecord));
            m_mapDataCollect.insert(std::make_pair(pReport->records(i).item(), std::move(mapKeyRecord)));
        }
        else
        {
            iter = data_iter->second.find(pReport->records(i).key());
            if (iter == data_iter->second.end())
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
                pRecord->set_key(pReport->records(i).key());
                pRecord->set_item(pReport->records(i).item());
                for (int j = 0; j < pReport->records(i).value_size(); ++j)
                {
                    pRecord->add_value(pReport->records(i).value(j));
                }
                data_iter->second.insert(std::make_pair(pReport->records(i).key(), pRecord));
            }
            else
            {
                for (int j = 0; j < pReport->records(i).value_size(); ++j)
                {
                    if (j < iter->second->value_size())
                    {
                        if (pReport->records(i).value_type() == ReportRecord::VALUE_ACC)
                        {
                            iter->second->set_value(j, iter->second->value(j) + pReport->records(i).value(j));
                        }
                        else
                        {
                            iter->second->set_value(j, pReport->records(i).value(j));
                        }
                    }
                    else
                    {
                        iter->second->add_value(pReport->records(i).value(j));
                    }
                }
            }
        }
    }
}

void SessionDataReport::AddReport(const std::string& strNodeIdentify, std::shared_ptr<Report> pReport)
{
    LOG4_TRACE("report from %s", strNodeIdentify.c_str());
    auto& mapUpdating = m_vecNodeReport[m_uiNodeReportUpdatingIndex];
    auto iter = mapUpdating.find(strNodeIdentify);
    if (iter == mapUpdating.end())
    {
        mapUpdating.insert(std::make_pair(strNodeIdentify, pReport));
    }
    else
    {
        iter->second = pReport;
    }
}

}

