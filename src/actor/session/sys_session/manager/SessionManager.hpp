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

namespace neb
{

class SessionManager : public Session,
    public DynamicCreator<SessionManager, bool>, public ActorSys
{
public:
    SessionManager(bool bDirectToLoader);
    virtual ~SessionManager();

    virtual E_CMD_STATUS Timeout();

    void AddOnlineNode(const std::string& strNodeIdentify, const std::string& strNodeInfo);
    void DelOnlineNode(const std::string& strNodeIdentify);
    bool SendToChild(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);    // 向Worker和Loader发送数据
    bool SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendToLoader(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    void AddWorkerInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd);
    void AddLoaderInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd);
    const WorkerInfo* GetWorkerInfo(int32 iWorkerIndex) const;
    bool SetWorkerLoad(int iWorkerFd, CJsonObject& oJsonLoad);
    int GetNextWorkerDataFd();
    std::pair<int, int> GetMinLoadWorkerDataFd();
    bool CheckWorker();
    bool WorkerDeath(int iPid, int& iWorkerIndex, Labor::LABOR_TYPE& eLaborType);
    void SendOnlineNodesToWorker();
    void MakeReportData(CJsonObject& oReportJson);
    int GetLoaderDataFd() const;
    bool NewSocketWhenWorkerCreated(int iWorkerDataFd);
    bool NewSocketWhenLoaderCreated();

private:
    bool m_bDirectToLoader = false;
    int m_iLoaderDataFd = -1;
    std::unordered_map<int, WorkerInfo*> m_mapWorker;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::unordered_map<int, WorkerInfo*>::iterator m_iterWorker;
    std::unordered_map<int, int> m_mapWorkerStartNum;       ///< 进程被启动次数，key为WorkerIdx
    std::unordered_map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号
    std::unordered_map<std::string, std::string> m_mapOnlineNodes;     ///< 订阅的节点在线信息
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SYS_SESSION_MANAGER_SESSIONMANAGER_HPP_ */

