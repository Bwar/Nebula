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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#ifdef __cplusplus
}
#endif

#include <memory>

#include "util/CBuffer.hpp"
#include "labor/Labor.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/RedisChannel.hpp"
#include "codec/Codec.hpp"
#include "logger/NetLogger.hpp"
#include "NodeInfo.hpp"

namespace neb
{

class Dispatcher;
class ActorBuilder;

class Worker: public Labor
{
public:
    Worker(const std::string& strWorkPath, int iControlFd, int iDataFd,
            int iWorkerIndex, Labor::LABOR_TYPE eLaborType = Labor::LABOR_WORKER);
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    virtual ~Worker();

    // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    void OnTerminated(struct ev_signal* watcher);
    bool CheckParent();

    virtual bool Init(CJsonObject& oJsonConf);
    void Run();

public:     // about worker
    virtual Dispatcher* GetDispatcher()
    {
        return(m_pDispatcher);
    }

    virtual ActorBuilder* GetActorBuilder()
    {
        return(m_pActorBuilder);
    }

    virtual uint32 GetSequence() const
    {
        ++m_ulSequence;
        if (0 == m_ulSequence)
        {
            ++m_ulSequence;
        }
        return(m_ulSequence);
    }

    virtual time_t GetNowTime() const;
    virtual const CJsonObject& GetNodeConf() const;
    virtual void SetNodeConf(const CJsonObject& oJsonConf);
    virtual const NodeInfo& GetNodeInfo() const;
    virtual void SetNodeId(uint32 uiNodeId);
    virtual bool AddNetLogMsg(const MsgBody& oMsgBody);
    const WorkerInfo& GetWorkerInfo() const;
    const CJsonObject& GetCustomConf() const;
    std::shared_ptr<SocketChannel> GetManagerControlChannel();
    bool SetCustomConf(const CJsonObject& oJsonConf);
    bool WithSsl();

    template <typename ...Targs>
        void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

protected:
    bool InitLogger(const CJsonObject& oJsonConf);
    virtual bool InitDispatcher();
    virtual bool InitActorBuilder();
    bool NewDispatcher();
    bool NewActorBuilder();
    bool CreateEvents();
    void Destroy();
    bool AddPeriodicTaskEvent();

private:
    char* m_pErrBuff = NULL;
    mutable uint32 m_ulSequence = 0;
    Dispatcher* m_pDispatcher = nullptr;
    ActorBuilder* m_pActorBuilder = nullptr;

    CJsonObject m_oNodeConf;
    CJsonObject m_oCustomConf;    ///< 自定义配置
    NodeInfo m_stNodeInfo;
    WorkerInfo m_stWorkerInfo;

    std::shared_ptr<NetLogger> m_pLogger = nullptr;
    std::shared_ptr<SocketChannel> m_pManagerControlChannel = nullptr;
    std::shared_ptr<SocketChannel> m_pManagerDataChannel = nullptr;
};

template <typename ...Targs>
void Worker::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}


} /* namespace neb */

#endif /* SRC_LABOR_WORKER_HPP_ */
