/*******************************************************************************
 * Project:  Nebula
 * @file     SessionManager.hpp
 * @brief    管理进程信息会话
 * @author   Bwar
 * @date:    2019年9月21日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_SESSION_SYS_SESSION_MANAGER_SESSIONMANAGER_HPP_
#define SRC_ACTOR_SESSION_SYS_SESSION_MANAGER_SESSIONMANAGER_HPP_

#include "actor/ActorSys.hpp"
#include "labor/NodeInfo.hpp"
#include "actor/session/Session.hpp"
#include "pb/report.pb.h"

namespace neb
{

class Loader;

class SessionManager : public Session,
    public DynamicCreator<SessionManager, ev_tstamp&>,
    public ActorSys
{
public:
    SessionManager(ev_tstamp dStatInterval);
    virtual ~SessionManager();

    virtual E_CMD_STATUS Timeout();

    void AddPort2WorkerInfo(uint32 uiWorkerNum, uint32 uiLoaderNum, CJsonObject& oPort2WorkerJson);
    void AddPortListenFd(int32 iPort, int32 iFd);
    void AddOnlineNode(const std::string& strNodeIdentify, const std::string& strNodeInfo);
    void DelOnlineNode(const std::string& strNodeIdentify);
    bool SendToChild(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);    // 向Worker和Loader发送数据
    bool SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendToLoader(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    void AddWorkerInfo(int iWorkerIndex, int iPid);
    Worker* MutableWorker(int iWorkerIndex, const std::string& strWorkPath);
    Loader* MutableLoader(int iWorkerIndex, const std::string& strWorkPath);
    std::shared_ptr<WorkerInfo> GetWorkerInfo(int32 iWorkerIndex) const;
    bool SetWorkerLoad(uint32 uiWorkerIndex, const ReportRecord& oWorkerStatus);
    void SetLoaderActorBuilder(ActorBuilder* pActorBuilder);
    int32 GetDispatchWorkerId(int32 iListenFd);
    int32 GetDispatchWorkerId(int32 iListenFd, const char* szRemoteAddr, int iRemotePort);
    bool CheckWorker();
    void SendOnlineNodesToWorker();
    void MakeReportData(CJsonObject& oReportJson);

private:
    uint32 m_uiRoundRobin = 0;
    std::vector<std::shared_ptr<WorkerInfo>> m_vecWorkerInfo;
    std::unordered_map<int, std::pair<uint32, std::vector<uint32>>> m_mapListenFd2Worker;

    std::unordered_map<int, Worker*> m_mapWorker;

    std::map<int32, std::vector<uint32>> m_mapPort2Worker;
    std::unordered_map<std::string, std::string> m_mapOnlineNodes;     ///< 订阅的节点在线信息
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SYS_SESSION_MANAGER_SESSIONMANAGER_HPP_ */

