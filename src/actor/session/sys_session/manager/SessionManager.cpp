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
#include "util/json/CJsonObject.hpp"
#include "labor/NodeInfo.hpp"
#include "labor/Manager.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/cmd/CW.hpp"

namespace neb
{

SessionManager::SessionManager()
    : Session("neb::SessionManager", gc_dNoTimeout)
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
    m_mapWorkerStartNum.clear();
    m_mapWorkerFdPid.clear();
    m_mapOnlineNodes.clear();
    m_vecFreeWorkerIdx.clear();
}

E_CMD_STATUS SessionManager::Timeout()
{
    return(CMD_STATUS_RUNNING);
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

bool SessionManager::SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (auto worker_iter = m_mapWorker.begin();
            worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        GetLabor(this)->GetDispatcher()->SendTo(worker_iter->second->iControlFd);
    }
    return(true);
}

void SessionManager::AddWorkerInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd)
{
    WorkerInfo* pWorkerInfo = nullptr;
    try
    {
        pWorkerInfo = new WorkerInfo();
    }
    catch (std::bad_alloc& e)
    {
        LOG4_ERROR("new WorkerInfo error: %s", e.what());
        return;
    }
    pWorkerInfo->iWorkerIndex = iWorkerIndex;
    pWorkerInfo->iControlFd = iControlFd;
    pWorkerInfo->iDataFd = iDataFd;
    pWorkerInfo->dBeatTime = GetNowTime();
    m_mapWorker.insert(std::make_pair(iPid, pWorkerInfo));
    m_mapWorkerFdPid.insert(std::make_pair(iControlFd, iPid));
    m_mapWorkerFdPid.insert(std::make_pair(iDataFd, iPid));
    auto start_num_iter = m_mapWorkerStartNum.find(iWorkerIndex);
    if (start_num_iter == m_mapWorkerStartNum.end())
    {
        m_mapWorkerStartNum.insert(std::make_pair(iWorkerIndex, 1));
    }
    else
    {
        start_num_iter->second++;
    }
}

const WorkerInfo* SessionManager::GetWorkerInfo(int32 iWorkerIndex) const
{
    for (auto worker_iter = m_mapWorker.begin();
            worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (iWorkerIndex == worker_iter->second->iWorkerIndex)
        {
            return(worker_iter->second);
        }
    }
    return(nullptr);
}

bool SessionManager::SetWorkerLoad(int iWorkerFd, CJsonObject& oJsonLoad)
{
    auto fd_pid_iter = m_mapWorkerFdPid.find(iWorkerFd);
    if (fd_pid_iter != m_mapWorkerFdPid.end())
    {
        auto iPid = fd_pid_iter->second;
        auto it = m_mapWorker.find(iPid);
        if (it != m_mapWorker.end())
        {
            oJsonLoad.Get("load", it->second->iLoad);
            oJsonLoad.Get("connect", it->second->iConnect);
            oJsonLoad.Get("recv_num", it->second->iRecvNum);
            oJsonLoad.Get("recv_byte", it->second->iRecvByte);
            oJsonLoad.Get("send_num", it->second->iSendNum);
            oJsonLoad.Get("send_byte", it->second->iSendByte);
            oJsonLoad.Get("client", it->second->iClientNum);
            it->second->dBeatTime = GetNowTime();
            return(true);
        }
        else
        {
            LOG4_ERROR("no worker info found for pid %d!", iPid);
            return(false);
        }
    }
    else
    {
        LOG4_ERROR("%d is not a worker fd!", iWorkerFd);
        return(false);
    }
}

std::pair<int, int> SessionManager::GetMinLoadWorkerDataFd()
{
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_fd;
    for (auto iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
    {
        if (iter == m_mapWorker.begin())
        {
            iMinLoadWorkerFd = iter->second->iDataFd;
            iMinLoad = iter->second->iLoad;
            worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
        }
        else if (iter->second->iLoad < iMinLoad)
        {
            iMinLoadWorkerFd = iter->second->iDataFd;
            iMinLoad = iter->second->iLoad;
            worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
        }
    }
    return(worker_pid_fd);
}

bool SessionManager::CheckWorker()
{
    for (auto worker_iter = m_mapWorker.begin();
            worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (GetNowTime() - worker_iter->second->dBeatTime
                > ((Manager*)GetLabor(this))->GetManagerInfo().iWorkerBeat)
        {
            LOG4_INFO("worker_%d pid %d is unresponsive, terminate it.",
                    worker_iter->second->iWorkerIndex, worker_iter->first);
            kill(worker_iter->first, SIGKILL);
        }
    }
    return(true);
}

bool SessionManager::WorkerDeath(int iPid, int& iWorkerIndex)
{
    auto worker_iter = m_mapWorker.find(iPid);
    if (worker_iter != m_mapWorker.end())
    {
        LOG4_TRACE("restart worker %d, close control fd %d and data fd %d first.",
                        worker_iter->second->iWorkerIndex, worker_iter->second->iControlFd, worker_iter->second->iDataFd);
        int iWorkerIndex = worker_iter->second->iWorkerIndex;
        auto fd_iter = m_mapWorkerFdPid.find(worker_iter->second->iControlFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        fd_iter = m_mapWorkerFdPid.find(worker_iter->second->iDataFd);
        if (fd_iter != m_mapWorkerFdPid.end())
        {
            m_mapWorkerFdPid.erase(fd_iter);
        }
        auto pControlChannel = GetLabor(this)->GetDispatcher()->GetChannel(worker_iter->second->iControlFd);
        GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pControlChannel);
        auto pDataChannel = GetLabor(this)->GetDispatcher()->GetChannel(worker_iter->second->iDataFd);
        GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pDataChannel);
        delete worker_iter->second;
        m_mapWorker.erase(worker_iter);

        auto restart_num_iter = m_mapWorkerStartNum.find(iWorkerIndex);
        if (restart_num_iter != m_mapWorkerStartNum.end())
        {
            LOG4_INFO("worker %d had been restarted %d times!", iWorkerIndex, restart_num_iter->second);
        }
        return(true);
    }
    else
    {
        return(false);
    }
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
    SendToWorker(CMD_REQ_NODE_NOTICE, GetSequence(), oMsgBody);
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
 *         "load":1885792, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":495870
 *     },
 *     "worker":
 *     [
 *          {"load":655666, "connect":495873, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":655235, "connect":485872, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}},
 *          {"load":585696, "connect":415379, "recv_num":98755266, "recv_byte":98856648832, "send_num":154846322, "send_byte":648469320222,"client":195870}}
 *     ]
 * }
 */
void SessionManager::MakeReportData(CJsonObject& oReportJson)
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
    CJsonObject oReportData;
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
    oReportData.Add("worker_num", (int)m_mapWorker.size());
    oReportData.Add("active_time", (uint64)GetNowTime());
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    auto worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        iLoad += worker_iter->second->iLoad;
        iConnect += worker_iter->second->iConnect;
        iRecvNum += worker_iter->second->iRecvNum;
        iRecvByte += worker_iter->second->iRecvByte;
        iSendNum += worker_iter->second->iSendNum;
        iSendByte += worker_iter->second->iSendByte;
        iClientNum += worker_iter->second->iClientNum;
        oMember.Clear();
        oMember.Add("load", worker_iter->second->iLoad);
        oMember.Add("connect", worker_iter->second->iConnect);
        oMember.Add("recv_num", worker_iter->second->iRecvNum);
        oMember.Add("recv_byte", worker_iter->second->iRecvByte);
        oMember.Add("send_num", worker_iter->second->iSendNum);
        oMember.Add("send_byte", worker_iter->second->iSendByte);
        oMember.Add("client", worker_iter->second->iClientNum);
        oReportData["worker"].Add(oMember);
    }
    oReportData["node"].Add("load", iLoad);
    oReportData["node"].Add("connect", iConnect);
    oReportData["node"].Add("recv_num", iRecvNum);
    oReportData["node"].Add("recv_byte", iRecvByte);
    oReportData["node"].Add("send_num", iSendNum);
    oReportData["node"].Add("send_byte", iSendByte);
    oReportData["node"].Add("client", iClientNum);
}

} /* namespace neb */
