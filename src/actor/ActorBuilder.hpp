/*******************************************************************************
 * Project:  Nebula
 * @file     ActorBuilder.hpp
 * @brief    Actor创建和管理
 * @author   Bwar
 * @date:    2019年9月15日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_ACTORBUILDER_HPP_
#define SRC_ACTOR_ACTORBUILDER_HPP_

#include <string>
#include <vector>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "ev.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "pb/neb_sys.pb.h"
#include "Definition.hpp"
#include "Error.hpp"
#include "ActorFactory.hpp"
#include "logger/NetLogger.hpp"

namespace neb
{

class Manager;
class Worker;

class Actor;
class Cmd;
class Module;
class Session;
class Timer;
class Context;
class Step;
class RedisStep;
class HttpStep;
class Model;
class Chain;
class SessionLogger;

class SocketChannel;
class RedisChannel;
class Dispatcher;

class CJsonObject;

class ActorBuilder
{
public:
    struct tagSo
    {
        void* pSoHandle = NULL;
        std::string strVersion;
        std::vector<int> vecCmd;
        std::vector<std::string> vecPath;
        tagSo(){};
        ~tagSo(){};
    };

public:
    ActorBuilder(Labor* pLabor, std::shared_ptr<NetLogger> pLogger);
    virtual ~ActorBuilder();
    bool Init(CJsonObject& oBootLoadConf, CJsonObject& oDynamicLoadConf);

    static void StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    static void ChainTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents);
    bool OnStepTimeout(std::shared_ptr<Step> pStep);
    bool OnSessionTimeout(std::shared_ptr<Session> pSession);
    bool OnChainTimeout(std::shared_ptr<Chain> pChain);
    bool OnMessage(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);
    bool OnMessage(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);
    bool OnRedisConnected(std::shared_ptr<RedisChannel> pChannel, const redisAsyncContext *c, int status);
    void OnRedisDisconnected(std::shared_ptr<RedisChannel> pChannel, const redisAsyncContext *c, int status);
    bool OnRedisCmdResult(std::shared_ptr<RedisChannel> pChannel, redisAsyncContext *c, void *reply, void *privdata);

public:
    template <typename ...Targs>
        void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);
    template <typename ...Targs>
        void Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Actor> MakeSharedActor(Actor* pCreator, const std::string& strActorName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Cmd> MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Module> MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Step> MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Session> MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Context> MakeSharedContext(Actor* pCreator, const std::string& strContextName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Model> MakeSharedModel(Actor* pCreator, const std::string& strModelName, Targs&&... args);

    template <typename ...Targs>
    std::shared_ptr<Chain> MakeSharedChain(Actor* pCreator, const std::string& strChainName, Targs&&... args);

    template <typename ...Targs> Actor* NewActor(const std::string& strActorName, Targs... args);
    std::shared_ptr<Actor> InitializeSharedActor(Actor* pCreator, std::shared_ptr<Actor> pSharedActor, const std::string& strActorName);
    bool TransformToSharedCmd(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedModule(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedStep(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedSession(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedModel(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);
    bool TransformToSharedChain(Actor* pCreator, std::shared_ptr<Actor> pSharedActor);

public:
    virtual std::shared_ptr<Session> GetSession(uint32 uiSessionId);
    virtual std::shared_ptr<Session> GetSession(const std::string& strSessionId);
    virtual bool ExecStep(uint32 uiStepSeq, int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);
    virtual std::shared_ptr<Model> GetModel(const std::string& strModelName);
    virtual bool ResetTimeout(std::shared_ptr<Actor> pSharedActor);
    int32 GetStepNum();
    bool ReloadCmdConf();
    bool AddNetLogMsg(const MsgBody& oMsgBody);

protected:
    void AddAssemblyLine(std::shared_ptr<Session> pSession);
    void RemoveStep(std::shared_ptr<Step> pStep);
    void RemoveSession(std::shared_ptr<Session> pSession);
    void RemoveChain(uint32 uiChainId);
    void ChannelNotice(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const std::string& strClientData);
    void ExecAssemblyLine(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    void AddChainConf(const std::string& strChainKey, std::queue<std::vector<std::string> >&& queChainBlocks);
    void LoadSysCmd();
    void BootLoadCmd(CJsonObject& oCmdConf);
    void DynamicLoad(CJsonObject& oSoConf);
    tagSo* LoadSo(const std::string& strSoPath, const std::string& strVersion);
    void LoadDynamicSymbol(CJsonObject& oOneSoConf);
    void UnloadDynamicSymbol(CJsonObject& oOneSoConf);

private:
    char* m_pErrBuff;
    Labor* m_pLabor;
    std::shared_ptr<NetLogger> m_pLogger;
    std::shared_ptr<SessionLogger> m_pSessionLogger;

    // dynamic load，use for load and unload.
    std::unordered_map<std::string, tagSo*> m_mapLoadedSo;
    std::unordered_map<std::string, std::unordered_set<int32> > m_mapLoadedCmd;             //key为CmdClassName，value为iCmd集合
    std::unordered_map<std::string, std::unordered_set<std::string> > m_mapLoadedModule;    //key为ModuleClassName，value为strModulePath集合
    std::unordered_map<std::string, std::unordered_set<uint32> > m_mapLoadedStep;           //key为StepClassName，value为uiSeq集合
    std::unordered_map<std::string, std::unordered_set<std::string> > m_mapLoadedSession;   //key为SessionClassName，value为strSessionId集合

    // Cmd and Module
    std::unordered_map<int32, std::shared_ptr<Cmd> > m_mapCmd;
    std::unordered_map<std::string, std::shared_ptr<Module> > m_mapModule;

    // Chain and Model
    std::unordered_map<std::string, std::queue<std::vector<std::string> > > m_mapChainConf; //key为Chain的配置名(ChainFlag)，value为由Model类名和Step类名构成的ChainBlock链
    std::unordered_map<uint32, std::shared_ptr<Chain> > m_mapChain;                         //key为Chain的Sequence，称为ChainId
    std::unordered_map<std::string, std::shared_ptr<Model> > m_mapModel;                  //key为Model类名

    // Step and Session
    std::unordered_map<uint32, std::shared_ptr<Step> > m_mapCallbackStep;
    std::unordered_map<std::string, std::shared_ptr<Session> > m_mapCallbackSession;
    std::unordered_set<std::shared_ptr<Session> > m_setAssemblyLine;   ///< 资源就绪后执行队列

    friend class Manager;
    friend class Worker;
    friend class Actor;
    friend class Dispatcher;
};

template <typename ...Targs>
void ActorBuilder::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
void ActorBuilder::Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Actor> ActorBuilder::MakeSharedActor(Actor* pCreator, const std::string& strActorName, Targs&&... args)
{
    Actor* pActor = ActorFactory<Targs...>::Instance()->Create(strActorName, std::forward<Targs>(args)...);
    if (nullptr == pActor)
    {
        /**
         * @brief 为兼容&&参数推导差异导致ActorFactory<Targs...>未Regist进而导致
         * ActorFactory<Targs...>::Instance()->Create()调用不到对应的创建函数而增加。
         * NewActor()参数将按值传递，如果调用到NewActor()才new成功，代价可能会相对大些。
         * 如果是整型、浮点型等内置类型则正常，如果是std::string等自定义类型，检查一下
         * Actor子类定义时public DynamicCreator传递的参数是否写错，导致本该按引用传递
         * 参数变成了按值传递。
         */
        pActor = NewActor(strActorName, std::forward<Targs>(args)...);
        if (nullptr == pActor)
        {
            LOG4_ERROR("failed to make shared actor \"%s\"", strActorName.c_str());
            return(nullptr);
        }
    }
    std::shared_ptr<Actor> pSharedActor;
    pSharedActor.reset(pActor);
    pActor = nullptr;
    return(InitializeSharedActor(pCreator, pSharedActor, strActorName));
}

template <typename ...Targs>
std::shared_ptr<Cmd> ActorBuilder::MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Cmd>(MakeSharedActor(pCreator, strCmdName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Module> ActorBuilder::MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Module>(MakeSharedActor(pCreator, strModuleName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Step> ActorBuilder::MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Step>(MakeSharedActor(pCreator, strStepName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Session> ActorBuilder::MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Session>(MakeSharedActor(pCreator, strSessionName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Context> ActorBuilder::MakeSharedContext(Actor* pCreator, const std::string& strContextName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Context>(MakeSharedActor(pCreator, strContextName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Model> ActorBuilder::MakeSharedModel(Actor* pCreator, const std::string& strModelName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Model>(MakeSharedActor(pCreator, strModelName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Chain> ActorBuilder::MakeSharedChain(Actor* pCreator, const std::string& strChainName, Targs&&... args)
{
    return(std::dynamic_pointer_cast<Chain>(MakeSharedActor(pCreator, strChainName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
Actor* ActorBuilder::NewActor(const std::string& strActorName, Targs... args)
{
    LOG4_TRACE("\"%s\" created by NewActor().", strActorName.c_str());
    return(ActorFactory<Targs...>::Instance()->Create(strActorName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_ACTORBUILDER_HPP_ */
