/*******************************************************************************
 * Project:  Nebula
 * @file     Manager.hpp
 * @brief    管理进程
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_MANAGER_HPP_
#define SRC_LABOR_MANAGER_HPP_

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <list>

#include "util/json/CJsonObject.hpp"
#include "logger/NetLogger.hpp"
#include "util/CBuffer.hpp"
#include "pb/msg.pb.h"
#include "pb/neb_sys.pb.h"
#include "Error.hpp"
#include "Definition.hpp"
#include "NodeInfo.hpp"
#include "Labor.hpp"
#include "channel/Channel.hpp"


namespace neb
{

class Worker;
class Dispatcher;
class ActorBuilder;
class Step;
class SessionManager;

class Manager: public Labor
{
public:
    struct tagManagerInfo
    {
        int iWorkerBeat     = 7;   ///< worker进程心跳，若大于此心跳未收到worker进程上报，则重启worker进程
        int iS2SListenFd    = -1;  ///< Server to Server监听文件描述符（Server与Server之间的连接较少，但每个Server的每个Worker均与其他Server的每个Worker相连）
        int iS2SFamily      = 0;   ///<
        int iC2SListenFd    = -1;  ///< Client to Server监听文件描述符（Client与Server之间的连接较多，但每个Client只需连接某个Server的某个Worker）
        int iC2SFamily      = 0;   ///<
    };

public:
    Manager(const std::string& strConfFile);
    virtual ~Manager();

    void Run();
    void OnTerminated(struct ev_signal* watcher);
    void OnChildTerminated(struct ev_signal* watcher);

    template <typename ...Targs>
        void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

public:
    virtual uint32 GetSequence() const
    {
        return(m_uiSequence++);
    }

    virtual Dispatcher* GetDispatcher()
    {
        return(m_pDispatcher);
    }

    virtual ActorBuilder* GetActorBuilder()
    {
        return(m_pActorBuilder);
    }

    std::shared_ptr<SessionManager> GetSessionManager()
    {
        return m_pSessionManager;
    }

    virtual time_t GetNowTime() const;
    virtual const CJsonObject& GetNodeConf() const;
    virtual void SetNodeConf(const CJsonObject& oNodeConf);
    virtual const NodeInfo& GetNodeInfo() const;
    virtual void SetNodeId(uint32 uiNodeId);
    const tagManagerInfo& GetManagerInfo() const;

    virtual bool AddNetLogMsg(const MsgBody& oMsgBody);
    void RefreshServer();

protected:
    bool GetConf();
    bool Init();
    bool InitLogger(const CJsonObject& oJsonConf);
    bool InitDispatcher();
    bool InitActorBuilder();
    void Destroy();

    bool CreateEvents();
    void CreateLoader();
    void CreateWorker();
    bool RestartWorker(int iDeathPid);
    bool AddPeriodicTaskEvent();

private:
    char* m_pErrBuff = NULL;
    mutable uint32 m_uiSequence = 0;
    Dispatcher* m_pDispatcher = nullptr;
    ActorBuilder* m_pActorBuilder = nullptr;

    CJsonObject m_oLastConf;          ///< 上次加载的配置
    CJsonObject m_oCurrentConf;       ///< 当前加载的配置
    NodeInfo m_stNodeInfo;
    tagManagerInfo m_stManagerInfo;
    ev_timer* m_pPeriodicTaskWatcher = NULL;          ///< 进程周期任务定时器
    std::shared_ptr<NetLogger> m_pLogger = nullptr;
    std::shared_ptr<SessionManager> m_pSessionManager = nullptr;
    std::shared_ptr<Step> m_pReportStep = nullptr;
};

template <typename ...Targs>
void Manager::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

} /* namespace neb */

#endif /* SRC_LABOR_MANAGER_HPP_ */
