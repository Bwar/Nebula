/*******************************************************************************
 * Project:  Nebula
 * @file     Engine.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月7日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_ENGINE_HPP_
#define SRC_ACTOR_ENGINE_HPP_

#include "labor/Worker.hpp"
#include "EngineModel.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Engine: public EngineModel
{
public:
    Engine()
        : EngineModel(Actor::ACT_ENGINE, gc_dNoTimeout)
    {
    }
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    virtual ~Engine(){}

    /**
     * @brief 提交
     */
    virtual E_CMD_STATUS Launch() = 0;

protected:
    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Engine> MakeSharedStep(const std::string& strEngineName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Cmd> MakeSharedCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Module> MakeSharedModule(const std::string& strModuleName, Targs... args);

private:
    friend class WorkerImpl;
};

template <typename ...Targs>
void Engine::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Engine> Engine::MakeSharedStep(const std::string& strEngineName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strEngineName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Engine::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Engine::MakeSharedCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->MakeSharedCmd(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Engine::MakeSharedModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strModuleName, std::forward<Targs>(args)...));
}


} /* namespace neb */

#endif /* SRC_ACTOR_ENGINE_HPP_ */
