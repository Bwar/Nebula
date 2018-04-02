/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.hpp
 * @brief 
 * @author   bwar
 * @date:    Feb 27, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_WORKER_HPP_
#define SRC_LABOR_WORKER_HPP_

#include "Labor.hpp"
#include "WorkerImpl.hpp"

namespace neb
{

class WorkerImpl;

class Worker: public Labor
{
public:
    Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf);
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    virtual ~Worker();

    // actor操作相关方法
    template <typename ...Targs> void Logger(const std::string& strTraceId, int iLogLevel, Targs... args);
    template <typename ...Targs> Step* NewStep(Actor* pCreator, const std::string& strStepName, Targs... args);
    template <typename ...Targs> Session* NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args);
    template <typename ...Targs> Cmd* NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args);
    template <typename ...Targs> Module* NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args);
    virtual uint32 GetSequence() const;
    virtual Session* GetSession(uint32 uiSessionId, const std::string& strSessionClass = "neb::Session");
    virtual Session* GetSession(const std::string& strSessionId, const std::string& strSessionClass = "neb::Session");

    // 获取worker信息相关方法
    virtual uint32 GetNodeId() const;
    virtual int GetWorkerIndex() const;
    virtual ev_tstamp GetDefaultTimeout() const;
    virtual int GetClientBeatTime() const;
    virtual const std::string& GetNodeType() const;
    virtual int GetPortForServer() const;
    virtual const std::string& GetHostForServer() const;
    virtual const std::string& GetWorkPath() const;
    virtual const std::string& GetNodeIdentify() const;
    virtual const CJsonObject& GetCustomConf() const;

    virtual time_t GetNowTime() const;

    // 网络IO相关方法
    virtual bool SendTo(const tagChannelContext& stCtx);
    virtual bool SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendPolling(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, unsigned int uiFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, RedisStep* pRedisStep);
    virtual std::string GetClientAddr(const tagChannelContext& stCtx);

    bool AddIoTimeout(const tagChannelContext& stCtx);

private:
    std::unique_ptr<WorkerImpl> m_pImpl;
    friend class WorkerImpl;
    friend class WorkerFriend;
};

template <typename ...Targs>
void Worker::Logger(const std::string& strTraceId, int iLogLevel, Targs... args)
{
    return(m_pImpl->Logger(strTraceId, iLogLevel, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Step* Worker::NewStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    return(m_pImpl->NewStep(pCreator, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Session* Worker::NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    return(m_pImpl->NewSession(pCreator, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Cmd* Worker::NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    return(m_pImpl->NewStep(pCreator, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Module* Worker::NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    return(m_pImpl->NewSession(pCreator, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_LABOR_WORKER_HPP_ */
