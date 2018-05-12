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

class Manager;
class WorkerImpl;
class Actor;

class Worker: public Labor
{
public:
    Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf);
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    virtual ~Worker();

    // actor操作相关方法
    template <typename ...Targs>
        void Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);

    template <typename ...Targs>
    std::shared_ptr<Step> MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs... args);

    template <typename ...Targs>
    std::shared_ptr<Session> MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs... args);

    template <typename ...Targs>
    std::shared_ptr<Cmd> MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs... args);

    template <typename ...Targs>
    std::shared_ptr<Module> MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs... args);

    virtual uint32 GetSequence() const;
    virtual std::shared_ptr<Session> GetSession(uint32 uiSessionId, const std::string& strSessionClass = "neb::Session");
    virtual std::shared_ptr<Session> GetSession(const std::string& strSessionId, const std::string& strSessionClass = "neb::Session");

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
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel);
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendPolling(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr);
    virtual bool SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq = 0);
    virtual bool SendTo(const std::string& strHost, int iPort, std::shared_ptr<RedisStep>  pRedisStep);

private:
    void Run();

private:
    WorkerImpl* m_pImpl;
    friend class WorkerImpl;
    friend class WorkerFriend;
    friend class Manager;
};

template <typename ...Targs>
void Worker::Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pImpl->Logger(strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Worker::MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    return(m_pImpl->MakeSharedStep(pCreator, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Worker::MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    return(m_pImpl->MakeSharedSession(pCreator, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Worker::MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    return(m_pImpl->MakeSharedStep(pCreator, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Worker::MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    return(m_pImpl->MakeSharedSession(pCreator, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_LABOR_WORKER_HPP_ */
