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

#include <thread>
#include <mutex>
#include "actor/ActorSys.hpp"
#include "labor/NodeInfo.hpp"
#include "actor/session/Session.hpp"

namespace neb
{

class Loader;

class SessionManager : public Session,
    public DynamicCreator<SessionManager, bool&, bool&, ev_tstamp&>,
    public ActorSys
{
public:
    SessionManager(bool bDirectToLoader, bool bDispatchWithPort, ev_tstamp dStatInterval);
    virtual ~SessionManager();

    virtual E_CMD_STATUS Timeout();

    void AddOnlineNode(const std::string& strNodeIdentify, const std::string& strNodeInfo);
    void DelOnlineNode(const std::string& strNodeIdentify);
    bool SendToChild(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);    // 向Worker和Loader发送数据
    bool SendToWorker(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendToLoader(int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    void AddWorkerInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd);
    void AddLoaderInfo(int iWorkerIndex, int iPid, int iControlFd, int iDataFd);
    Worker* MutableWorker(int iWorkerIndex, const std::string& strWorkPath, int iControlFd, int iDataFd);
    Loader* MutableLoader(int iWorkerIndex, const std::string& strWorkPath, int iControlFd, int iDataFd);
    const WorkerInfo* GetWorkerInfo(int32 iWorkerIndex) const;
    bool SetWorkerLoad(int iWorkerFd, CJsonObject& oJsonLoad);
    void SetLoaderActorBuilder(ActorBuilder* pActorBuilder);
    int GetWorkerDataFd(const char* szClientAddr, int iClientPort, int iBonding);
    int GetNextWorkerDataFd(int iBonding);
    int GetMinLoadWorkerDataFd(int iBonding);
    bool CheckWorker();
    bool WorkerDeath(int iPid, int& iWorkerIndex, Labor::LABOR_TYPE& eLaborType);
    void SendOnlineNodesToWorker();
    void MakeReportData(CJsonObject& oReportJson);
    int GetLoaderDataFd() const;
    bool NewSocketWhenWorkerCreated(int iWorkerDataFd);
    bool NewSocketWhenLoaderCreated();

    static const std::vector<uint64>& GetWorkerThreadId()
    {
        return(s_vecWorkerThreadId);
    }
    static void AddWorkerThreadId(uint64 ullThreadId);

private:
    bool m_bDirectToLoader = false;
    bool m_bDispatchWithPort = false;
    int m_iLoaderDataFd = -1;
    uint32 m_uiWorkerDataFdIndex[2] = {0};  ///< for round robin fd transfer
    std::vector<int> m_vecWorkerDataFd;     ///< only for fd transfer
    std::unordered_map<int, Worker*> m_mapWorker;               ///< only thread worker
    std::unordered_map<int, WorkerInfo*> m_mapWorkerInfo;       ///< 业务逻辑工作进程及进程属性，key为pid
    std::unordered_map<int, int> m_mapWorkerStartNum;       ///< 进程被启动次数，key为WorkerIdx
    std::unordered_map<int, int> m_mapWorkerFdPid;            ///< 工作进程通信FD对应的进程号
    std::unordered_map<std::string, std::string> m_mapOnlineNodes;     ///< 订阅的节点在线信息

    static std::mutex s_mutexWorker;
    static std::vector<uint64> s_vecWorkerThreadId;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SYS_SESSION_MANAGER_SESSIONMANAGER_HPP_ */

