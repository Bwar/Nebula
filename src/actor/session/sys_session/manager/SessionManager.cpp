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
#include "util/process_helper.h"
#include "util/json/CJsonObject.hpp"
#include "labor/NodeInfo.hpp"
#include "labor/Manager.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/cmd/CW.hpp"

namespace neb
{

SessionManager::SessionManager(bool bDirectToLoader)
    : Session("neb::SessionManager", gc_dNoTimeout), m_bDirectToLoader(bDirectToLoader)
{
    m_iterWorker = m_mapWorker.begin();
}

SessionManager::~SessionManager()
{
    for (auto it = m_mapWorker.begin(); it != m_mapWorker.end(); ++it)
    {
        delete it->second;
        it->second = nullptr;
    }
    m_mapWorker.clear();
    m_iterWorker = m_mapWorker.begin();
    m_mapWorkerStartNum.clear();
    m_mapWorkerFdPid.clear();
    m_mapOnlineNodes.clear();
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

bool SessionManager::SendToChild(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (auto worker_iter = m_mapWorker.begin(); worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        GetLabor(this)->GetDispatcher()->SendTo(
                worker_iter->second->iControlFd, iCmd, uiSeq, oMsgBody);
    }
    return(true);
}

bool SessionManager::SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (auto worker_iter = m_mapWorker.begin(); worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (m_iLoaderDataFd == worker_iter->second->iDataFd)
        {
            continue;
        }
        GetLabor(this)->GetDispatcher()->SendTo(
                worker_iter->second->iControlFd, iCmd, uiSeq, oMsgBody);
    }
    return(true);
}

bool SessionManager::SendToLoader(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    for (auto worker_iter = m_mapWorker.begin(); worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (m_iLoaderDataFd == worker_iter->second->iDataFd)
        {
            GetLabor(this)->GetDispatcher()->SendTo(
                    worker_iter->second->iControlFd, iCmd, uiSeq, oMsgBody);
        }
    }
    return(true);
}

void SessionManager::AddWorkerInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd)
{
    WorkerInfo* pWorkerAttr = nullptr;
    try
    {
        pWorkerAttr = new WorkerInfo();
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new WorkerInfo error: %s", e.what());
        return;
    }
    pWorkerAttr->iWorkerIndex = iWorkerIndex;
    pWorkerAttr->iControlFd = iControlFd;
    pWorkerAttr->iDataFd = iDataFd;
    pWorkerAttr->dBeatTime = GetNowTime();
    m_mapWorker.insert(std::make_pair(iPid, pWorkerAttr));
    m_iterWorker = m_mapWorker.begin();
    m_mapWorkerFdPid.insert(std::pair<int, int>(iControlFd, iPid));
    m_mapWorkerFdPid.insert(std::pair<int, int>(iDataFd, iPid));
    auto start_num_iter = m_mapWorkerStartNum.find(iWorkerIndex);
    if (start_num_iter == m_mapWorkerStartNum.end())
    {
        m_mapWorkerStartNum.insert(std::pair<int, int>(iWorkerIndex, 1));
    }
    else
    {
        start_num_iter->second++;
    }
}

void SessionManager::AddLoaderInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd)
{
    m_iLoaderDataFd = iDataFd;
    AddWorkerInfo(iWorkerIndex, iPid, iControlFd, iDataFd);
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
            it->second->bStartBeatCheck = true;
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

int SessionManager::GetNextWorkerDataFd()
{
    if (m_bDirectToLoader && m_iLoaderDataFd != -1)
    {
        return(m_iLoaderDataFd);
    }
    else
    {
        if (m_mapWorker.empty())
        {
            return(-1);
        }
        ++m_iterWorker;
        if (m_iterWorker == m_mapWorker.end())
        {
            m_iterWorker = m_mapWorker.begin();
        }
        return(m_iterWorker->second->iDataFd);
    }
}

std::pair<int, int> SessionManager::GetMinLoadWorkerDataFd()
{
    LOG4_TRACE(" ");
    int iMinLoadWorkerFd = 0;
    int iMinLoad = -1;
    std::pair<int, int> worker_pid_fd;
    if (m_bDirectToLoader && m_iLoaderDataFd != -1)
    {
        auto it = m_mapWorkerFdPid.find(m_iLoaderDataFd);
        if (it != m_mapWorkerFdPid.end())
        {
            worker_pid_fd = std::pair<int, int>(it->second, m_iLoaderDataFd);
        }
        else
        {
            worker_pid_fd = std::pair<int, int>(0, m_iLoaderDataFd);
        }
    }
    else
    {
        for (auto iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
        {
            if (iMinLoad == -1 && iter->second->iDataFd != m_iLoaderDataFd)
            {
               iMinLoadWorkerFd = iter->second->iDataFd;
               iMinLoad = iter->second->iLoad;
               worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
            }
            else if (iter->second->iLoad < iMinLoad && iter->second->iDataFd != m_iLoaderDataFd)
            {
               iMinLoadWorkerFd = iter->second->iDataFd;
               iMinLoad = iter->second->iLoad;
               worker_pid_fd = std::pair<int, int>(iter->first, iMinLoadWorkerFd);
            }
        }
    }
    return(worker_pid_fd);
}

bool SessionManager::CheckWorker()
{
    LOG4_TRACE(" ");
    for (auto worker_iter = m_mapWorker.begin();
                    worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (worker_iter->second->bStartBeatCheck)
        {
            LOG4_TRACE("now %lf, worker_beat_time %lf, worker_beat %d",
                    GetNowTime(), worker_iter->second->dBeatTime, ((Manager*)GetLabor(this))->GetManagerInfo().iWorkerBeat);
            if (GetNowTime() - worker_iter->second->dBeatTime > ((Manager*)GetLabor(this))->GetManagerInfo().iWorkerBeat)
            {
                LOG4_ERROR("worker_%d pid %d is unresponsive, "
                            "terminate it.", worker_iter->second->iWorkerIndex, worker_iter->first);
                kill(worker_iter->first, SIGKILL); //SIGINT);
            }
        }
    }
    return(true);
}

bool SessionManager::WorkerDeath(int iPid, int& iWorkerIndex, Labor::LABOR_TYPE& eLaborType)
{
    auto worker_iter = m_mapWorker.find(iPid);
    if (worker_iter != m_mapWorker.end())
    {
        LOG4_TRACE("restart worker %d, close control fd %d and data fd %d first.",
                        worker_iter->second->iWorkerIndex, worker_iter->second->iControlFd, worker_iter->second->iDataFd);
        iWorkerIndex = worker_iter->second->iWorkerIndex;
        if (m_iLoaderDataFd == worker_iter->second->iDataFd)
        {
            eLaborType = Labor::LABOR_LOADER;
        }
        else
        {
            eLaborType = Labor::LABOR_WORKER;
        }
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
        GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pControlChannel); // 在io事件中已关闭连接，这里可以不需要
        auto pDataChannel = GetLabor(this)->GetDispatcher()->GetChannel(worker_iter->second->iDataFd);
        GetLabor(this)->GetDispatcher()->DiscardSocketChannel(pDataChannel);
        delete worker_iter->second;
        m_mapWorker.erase(worker_iter);
        m_iterWorker = m_mapWorker.begin();

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
void SessionManager::MakeReportData(CJsonObject& oReportData)
{
    int iLoad = 0;
    int iConnect = 0;
    int iRecvNum = 0;
    int iRecvByte = 0;
    int iSendNum = 0;
    int iSendByte = 0;
    int iClientNum = 0;
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
    if (m_iLoaderDataFd == -1)
    {
        oReportData.Add("worker_num", (int)m_mapWorker.size());
    }
    else
    {
        oReportData.Add("worker_num", (int)m_mapWorker.size() - 1);
    }
    oReportData.Add("active_time", (uint64)GetNowTime());
    oReportData.Add("node", CJsonObject("{}"));
    oReportData.Add("worker", CJsonObject("[]"));
    auto worker_iter = m_mapWorker.begin();
    for (; worker_iter != m_mapWorker.end(); ++worker_iter)
    {
        if (m_iLoaderDataFd == worker_iter->second->iDataFd)
        {
            continue;
        }
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

int SessionManager::GetLoaderDataFd() const
{
    return(m_iLoaderDataFd);
}

bool SessionManager::NewSocketWhenWorkerCreated(int iWorkerDataFd)
{
    if (m_iLoaderDataFd == -1)
    {
        LOG4_TRACE("no Loader process.");
        return(true);
    }
    int iFds[2];
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, iFds) < 0)
    {
        LOG4_ERROR("socketpair error %d!", errno);
        return(false);
    }
    x_sock_set_block(iFds[0], 0);
    x_sock_set_block(iFds[1], 0);
    GetLabor(this)->GetDispatcher()->SendFd(m_iLoaderDataFd, iFds[0], PF_UNIX, CODEC_NEBULA_IN_NODE);
    GetLabor(this)->GetDispatcher()->SendFd(iWorkerDataFd, iFds[1], PF_UNIX, CODEC_NEBULA_IN_NODE);
    return(true);
}

bool SessionManager::NewSocketWhenLoaderCreated()
{
    if (m_iLoaderDataFd == -1)
    {
        LOG4_TRACE("no Loader process.");
        return(true);
    }
    for (auto iter = m_mapWorker.begin(); iter != m_mapWorker.end(); ++iter)
    {
        if (m_iLoaderDataFd == iter->second->iDataFd)
        {
            continue;
        }
        int iFds[2];
        if (socketpair(PF_UNIX, SOCK_STREAM, 0, iFds) < 0)
        {
            LOG4_ERROR("socketpair error %d!", errno);
            return(false);
        }
        x_sock_set_block(iFds[0], 0);
        x_sock_set_block(iFds[1], 0);
        GetLabor(this)->GetDispatcher()->SendFd(m_iLoaderDataFd, iFds[0], PF_UNIX, CODEC_NEBULA_IN_NODE);
        GetLabor(this)->GetDispatcher()->SendFd(iter->second->iDataFd, iFds[1], PF_UNIX, CODEC_NEBULA_IN_NODE);
    }
    return(true);
}

} /* namespace neb */
