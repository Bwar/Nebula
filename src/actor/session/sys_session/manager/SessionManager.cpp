/*******************************************************************************
 * Project:  Nebula
 * @file     SessionManager.cpp
 * @brief    管理进程信息会话
 * @author   bwar
 * @date:    2019年9月21日
 * @note
 * Modify history:
 ******************************************************************************/

#include "SessionManager.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <algorithm>
#include "pb/report.pb.h"
#include "util/process_helper.h"
#include "util/json/CJsonObject.hpp"
#include "util/encrypt/city.h"
#include "util/StringConverter.hpp"
#include "labor/NodeInfo.hpp"
#include "labor/Manager.hpp"
#include "labor/Worker.hpp"
#include "labor/Loader.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/IO.hpp"
#include "actor/cmd/CW.hpp"
#include "actor/session/sys_session/SessionDataReport.hpp"

namespace neb
{

SessionManager::SessionManager(ev_tstamp dStatInterval)
    : Session("neb::SessionManager", dStatInterval)
{
}

SessionManager::~SessionManager()
{
    for (auto it = m_mapWorker.begin(); it != m_mapWorker.end(); ++it)
    {
        delete it->second;
        it->second = nullptr;
    }
    m_mapWorker.clear();
    m_vecWorkerInfo.clear();
    m_mapOnlineNodes.clear();
}

E_CMD_STATUS SessionManager::Timeout()
{
    std::shared_ptr<Report> pReport = std::make_shared<Report>();
    if (pReport == nullptr)
    {
        LOG4_ERROR("failed to new Report!");
        return(CMD_STATUS_FAULT);
    }
    neb::ReportRecord* pRecord = nullptr;
    uint32 uiLoad = 0;
    uint32 uiConnect = 0;
    char szKey[256] = {0};
    for (uint32 i = 0; i < m_vecWorkerInfo.size(); ++i)
    {
        if (m_vecWorkerInfo[i] == nullptr)
        {
            continue;
        }
        uiLoad += m_vecWorkerInfo[i]->uiLoad;
        uiConnect += m_vecWorkerInfo[i]->uiConnection;
        pRecord = pReport->add_records();
        snprintf(szKey, 256, "W%u-recv_num", i);
        pRecord->set_key(szKey);
        pRecord->set_item("nebula");
        pRecord->add_value(m_vecWorkerInfo[i]->uiRecvNum);
        pRecord->set_value_type(ReportRecord::VALUE_ACC);
        pRecord = pReport->add_records();
        snprintf(szKey, 256, "W%u-recv_bytes", i);
        pRecord->set_key(szKey);
        pRecord->set_item("nebula");
        pRecord->add_value(m_vecWorkerInfo[i]->uiRecvByte);
        pRecord->set_value_type(ReportRecord::VALUE_ACC);
        pRecord = pReport->add_records();
        snprintf(szKey, 256, "W%u-send_num", i);
        pRecord->set_key(szKey);
        pRecord->set_item("nebula");
        pRecord->add_value(m_vecWorkerInfo[i]->uiSendNum);
        pRecord->set_value_type(ReportRecord::VALUE_ACC);
        pRecord = pReport->add_records();
        snprintf(szKey, 256, "W%u-send_bytes", i);
        pRecord->set_key(szKey);
        pRecord->set_item("nebula");
        pRecord->add_value(m_vecWorkerInfo[i]->uiSendByte);
        pRecord->set_value_type(ReportRecord::VALUE_ACC);
    }
    pRecord = pReport->add_records();
    pRecord->set_key("load");
    pRecord->set_item("nebula");
    pRecord->add_value(uiLoad);
    pRecord->set_value_type(ReportRecord::VALUE_FIXED);
    pRecord = pReport->add_records();
    pRecord->set_key("connect");
    pRecord->set_item("nebula");
    pRecord->add_value(uiConnect);
    pRecord->set_value_type(ReportRecord::VALUE_FIXED);
    std::string strSessionId = "neb::SessionDataReport";
    auto pSharedSession = GetSession(strSessionId);
    if (pSharedSession == nullptr)
    {
        LOG4_ERROR("no session named \"neb::SessionDataReport\"!");
        return(CMD_STATUS_RUNNING);
    }
    auto pReportSession = std::dynamic_pointer_cast<SessionDataReport>(pSharedSession);
    pReportSession->AddReport(pReport);
    return(CMD_STATUS_RUNNING);
}

void SessionManager::AddPort2WorkerInfo(uint32 uiWorkerNum, uint32 uiLoaderNum, CJsonObject& oPort2WorkerJson)
{
    uint32 uiWorkerId = 0;
    int32 iPort = 0;
    std::string strPort;
    while(oPort2WorkerJson.GetKey(strPort))
    {
        std::vector<uint32> vecWorkerId;
        iPort = StringConverter::RapidAtoi<int32>(strPort.c_str());
        for (int i = 0; i < oPort2WorkerJson[strPort].GetArraySize(); ++i)
        {
            if (oPort2WorkerJson[strPort].Get(i, uiWorkerId))
            {
                if (uiWorkerId <= uiWorkerNum)
                {
                    if (uiLoaderNum > 0)
                    {
                        vecWorkerId.push_back(uiWorkerId);
                    }
                    else
                    {
                        if (uiWorkerId != 0)
                        {
                            vecWorkerId.push_back(uiWorkerId);
                        }
                    }
                }
            }
        }
        m_mapPort2Worker.insert(std::make_pair(iPort, vecWorkerId));
    }
}

void SessionManager::AddPortListenFd(int32 iPort, int32 iFd)
{
    auto port_iter = m_mapPort2Worker.find(iPort);
    if (port_iter == m_mapPort2Worker.end())
    {
        ;   // no this port to worker config
    }
    else
    {
        auto& vecWorkerId = port_iter->second;
        auto fd_iter = m_mapListenFd2Worker.find(iFd);
        if (fd_iter == m_mapListenFd2Worker.end())
        {
            m_mapListenFd2Worker.insert(std::make_pair(iFd, std::make_pair(0, vecWorkerId)));
        }
    }
}

void SessionManager::AddOnlineNode(const std::string& strNodeIdentify, const std::string& strNodeInfo)
{
    auto iter = m_mapOnlineNodes.find(strNodeIdentify);
    if (iter == m_mapOnlineNodes.end())
    {
        m_mapOnlineNodes.insert(
                std::make_pair(strNodeIdentify, strNodeInfo));
    }
    else
    {
        iter->second = strNodeInfo;
    }
}

void SessionManager::DelOnlineNode(const std::string& strNodeIdentify)
{
    auto iter = m_mapOnlineNodes.find(strNodeIdentify);
    if (iter == m_mapOnlineNodes.end())
    {
        m_mapOnlineNodes.erase(iter);
    }
}

bool SessionManager::SendToChild(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (uint32 i = 0; i < m_vecWorkerInfo.size(); ++i)
    {
        if (m_vecWorkerInfo[i] == nullptr)
        {
            continue;
        }
        MsgHead oMsgHead;   // It will be cleared after each successful sending
        MsgBody oOutMsgBody(oMsgBody);
        oMsgHead.set_cmd(iCmd);
        oMsgHead.set_seq(uiSeq);
        IO<CodecNebulaInNode>::TransmitTo(this, m_vecWorkerInfo[i]->iWorkerIndex, uiSeq, oMsgHead, oMsgBody);
    }
    return(true);
}

bool SessionManager::SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (uint32 i = 1; i < m_vecWorkerInfo.size(); ++i)
    {
        if (m_vecWorkerInfo[i] == nullptr)
        {
            continue;
        }
        MsgHead oMsgHead;   // It will be cleared after each successful sending
        MsgBody oOutMsgBody(oMsgBody);
        oMsgHead.set_cmd(iCmd);
        oMsgHead.set_seq(uiSeq);
        IO<CodecNebulaInNode>::TransmitTo(this, m_vecWorkerInfo[i]->iWorkerIndex, uiSeq, oMsgHead, oMsgBody);
    }
    return(true);
}

bool SessionManager::SendToLoader(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    if (m_vecWorkerInfo.size() == 0)
    {
        LOG4_ERROR("no worker info.");
        return(false);
    }
    if (m_vecWorkerInfo[0] == nullptr)
    {
        LOG4_ERROR("no loader.");
        return(false);
    }
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    return(IO<CodecNebulaInNode>::TransmitTo(this, 0, uiSeq, oMsgHead, oMsgBody));
}

void SessionManager::AddWorkerInfo(int iWorkerIndex, int iPid)
{
    if (iWorkerIndex < 0)
    {
        LOG4_ERROR("invalid worker index %d", iWorkerIndex);
        return;
    }
    for (uint32 i = m_vecWorkerInfo.size(); i <= (uint32)iWorkerIndex; ++i)
    {
        m_vecWorkerInfo.push_back(nullptr);
    }
    auto pWorkerAttr = std::make_shared<WorkerInfo>();
    pWorkerAttr->iWorkerIndex = iWorkerIndex;
    pWorkerAttr->iWorkerPid = iPid;
    pWorkerAttr->dBeatTime = GetNowTime();
    m_vecWorkerInfo[iWorkerIndex] = pWorkerAttr;
}

Worker* SessionManager::MutableWorker(int iWorkerIndex, const std::string& strWorkPath)
{
    auto iter = m_mapWorker.find(iWorkerIndex);
    if (iter == m_mapWorker.end())
    {
        Worker* pWorker = nullptr;
        try
        {
            pWorker = new Worker(strWorkPath, iWorkerIndex);
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new Worker error: %s", e.what());
            return(nullptr);
        }
        m_mapWorker.insert(std::make_pair(iWorkerIndex, pWorker));
        return(pWorker);
    }
    else
    {
        return(iter->second);
    }
}

Loader* SessionManager::MutableLoader(int iWorkerIndex, const std::string& strWorkPath)
{
    auto iter = m_mapWorker.find(iWorkerIndex);
    if (iter == m_mapWorker.end())
    {
        Loader* pLoader = nullptr;
        try
        {
            pLoader = new Loader(strWorkPath, iWorkerIndex);
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new Worker error: %s", e.what());
            return(nullptr);
        }
        m_mapWorker.insert(std::make_pair(iWorkerIndex, (Worker*)pLoader));
        return(pLoader);
    }
    else
    {
        return((Loader*)iter->second);
    }
}

std::shared_ptr<WorkerInfo> SessionManager::GetWorkerInfo(int32 iWorkerIndex) const
{
    if (iWorkerIndex < 0 || iWorkerIndex >= (int32)m_vecWorkerInfo.size())
    {
        return(nullptr);
    }
    return(m_vecWorkerInfo[iWorkerIndex]);
}

bool SessionManager::SetWorkerLoad(uint32 uiWorkerIndex, const ReportRecord& oWorkerStatus)
{
    if (uiWorkerIndex >= m_vecWorkerInfo.size())
    {
        return(false);
    }
    if (m_vecWorkerInfo[uiWorkerIndex] == nullptr)
    {
        return(false);
    }
    if (oWorkerStatus.value_size() < 6)
    {
        LOG4_ERROR("value size %d error", oWorkerStatus.value_size());
        return(false);
    }
    m_vecWorkerInfo[uiWorkerIndex]->uiLoad = oWorkerStatus.value(0);
    m_vecWorkerInfo[uiWorkerIndex]->uiConnection = oWorkerStatus.value(1);
    m_vecWorkerInfo[uiWorkerIndex]->uiRecvNum = oWorkerStatus.value(2);
    m_vecWorkerInfo[uiWorkerIndex]->uiRecvByte = oWorkerStatus.value(3);
    m_vecWorkerInfo[uiWorkerIndex]->uiSendNum = oWorkerStatus.value(4);
    m_vecWorkerInfo[uiWorkerIndex]->uiSendByte = oWorkerStatus.value(5);
    return(true);
}

void SessionManager::SetLoaderActorBuilder(ActorBuilder* pActorBuilder)
{
    for (auto it = m_mapWorker.begin(); it != m_mapWorker.end(); ++it)
    {
        it->second->SetLoaderActorBuilder(pActorBuilder);
    }
}

int32 SessionManager::GetDispatchWorkerId(int32 iListenFd)
{
    auto fd_iter = m_mapListenFd2Worker.find(iListenFd);
    if (fd_iter == m_mapListenFd2Worker.end())  // no bonding
    {
        ++m_uiRoundRobin;
        if (m_uiRoundRobin > m_vecWorkerInfo.size() - 1)
        {
            m_uiRoundRobin = 1;
        }
        if (m_uiRoundRobin > m_vecWorkerInfo.size() - 1)
        {
            return(-1); // no worker
        }
        return(m_vecWorkerInfo[m_uiRoundRobin]->iWorkerIndex);
    }
    else
    {
        fd_iter->second.first++;
        if (fd_iter->second.first >= fd_iter->second.second.size())
        {
            fd_iter->second.first = 0;
        }
        return(fd_iter->second.second[fd_iter->second.first]);
    }
}

int32 SessionManager::GetDispatchWorkerId(int32 iListenFd, const char* szRemoteAddr, int iRemotePort)
{
    char szIdentify[64] = {0};
    snprintf(szIdentify, 64, "%s:%d", szRemoteAddr, iRemotePort);
    uint32 uiKeyHash = CityHash32(szIdentify, strlen(szIdentify));
    auto fd_iter = m_mapListenFd2Worker.find(iListenFd);
    if (fd_iter == m_mapListenFd2Worker.end())  // no bonding
    {
        uint32 uiIndex = uiKeyHash % (m_vecWorkerInfo.size() - 1) + 1;    // loader的worker编号为0
        return(m_vecWorkerInfo[uiIndex]->iWorkerIndex);
    }
    else
    {
        uint32 uiIndex = uiKeyHash % fd_iter->second.second.size();    // loader的worker编号为0
        return(fd_iter->second.second[uiIndex]);
    }
}

bool SessionManager::CheckWorker()
{
    LOG4_TRACE(" ");
    for (uint32 i = 0; i < m_vecWorkerInfo.size(); ++i)
    {
        if (m_vecWorkerInfo[i] == nullptr)
        {
            continue;
        }
        if (m_vecWorkerInfo[i]->bStartBeatCheck)
        {
            LOG4_TRACE("now %lf, worker_beat_time %lf, worker_beat %d",
                    GetNowTime(), m_vecWorkerInfo[i]->dBeatTime, GetLabor(this)->GetNodeInfo().dDataReportInterval);
            if (GetNowTime() - m_vecWorkerInfo[i]->dBeatTime > GetLabor(this)->GetNodeInfo().dDataReportInterval)
            {
                LOG4_ERROR("worker_%d pid %d is unresponsive.", m_vecWorkerInfo[i]->iWorkerIndex, m_vecWorkerInfo[i]->iWorkerPid);
            }
        }
    }
    return(true);
}

void SessionManager::SendOnlineNodesToWorker()
{
    // 重启Worker进程后下发其他节点的信息
    MsgBody oMsgBody;
    CJsonObject oSubscribeNode;
    oSubscribeNode.Add("add_nodes", CJsonObject("[]"));
    for (auto it = m_mapOnlineNodes.begin(); it != m_mapOnlineNodes.end(); ++it)
    {
        oSubscribeNode["add_nodes"].Add(CJsonObject(it->second));
    }
    oMsgBody.set_data(oSubscribeNode.ToString());
    SendToChild(CMD_REQ_NODE_NOTICE, GetSequence(), oMsgBody);
}

/**
 * @brief 上报节点状态信息
 * @return 上报是否成功
 * @note 节点状态信息结构如：
 * {
 *     "node_type":"ACCESS",
 *     "node_ip":"192.168.11.12",
 *     "node_port":9988,
 *     "access_ip":"120.234.2.106",
 *     "access_port":10001,
 *     "worker_num":10,
 *     "active_time":16879561651.06,
 *     "node":{
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222}}
 *     ]
 * }
 */
void SessionManager::MakeReportData(CJsonObject& oReportData)
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    CJsonObject oMember;
    oReportData.Add("node_type", GetLabor(this)->GetNodeInfo().strNodeType);
    oReportData.Add("node_id", GetLabor(this)->GetNodeInfo().uiNodeId);
    oReportData.Add("node_ip", GetLabor(this)->GetNodeInfo().strHostForServer);
    oReportData.Add("node_port", GetLabor(this)->GetNodeInfo().iPortForServer);
    if (GetLabor(this)->GetNodeInfo().strGateway.size() > 0)
    {
        oReportData.Add("access_ip", GetLabor(this)->GetNodeInfo().strGateway);
    }
    else
    {
        oReportData.Add("access_ip", GetLabor(this)->GetNodeInfo().strHostForClient);
    }
    if (GetLabor(this)->GetNodeInfo().iGatewayPort > 0)
    {
        oReportData.Add("access_port", GetLabor(this)->GetNodeInfo().iGatewayPort);
    }
    else
    {
        oReportData.Add("access_port", GetLabor(this)->GetNodeInfo().iPortForClient);
    }
    if (m_vecWorkerInfo.size() > 0 && m_vecWorkerInfo[0] == nullptr)    // without Loader
    {
        oReportData.Add("worker_num", (int)m_vecWorkerInfo.size() - 1);
    }
    else
    {
        oReportData.Add("worker_num", (int)m_vecWorkerInfo.size());
    }
    oReportData.Add("active_time", (uint64)GetNowTime());
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    for (uint32 i = 0; i < m_vecWorkerInfo.size(); ++i)
    {
        if (m_vecWorkerInfo[i] == nullptr)
        {
            continue;
        }
        iLoad += m_vecWorkerInfo[i]->uiLoad;
        iConnect += m_vecWorkerInfo[i]->uiConnection;
        iRecvNum += m_vecWorkerInfo[i]->uiRecvNum;
        iRecvByte += m_vecWorkerInfo[i]->uiRecvByte;
        iSendNum += m_vecWorkerInfo[i]->uiSendNum;
        iSendByte += m_vecWorkerInfo[i]->uiSendByte;
        oMember.Clear();
        oMember.Add("load", m_vecWorkerInfo[i]->uiLoad);
        oMember.Add("connect", m_vecWorkerInfo[i]->uiConnection);
        oMember.Add("recv_num", m_vecWorkerInfo[i]->uiRecvNum);
        oMember.Add("recv_byte", m_vecWorkerInfo[i]->uiRecvByte);
        oMember.Add("send_num", m_vecWorkerInfo[i]->uiSendNum);
        oMember.Add("send_byte", m_vecWorkerInfo[i]->uiSendByte);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
}

} /* namespace neb */

