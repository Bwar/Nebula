/*******************************************************************************
 * Project:  Nebula
 * @file     Step.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_STEP_HPP_
#define SRC_ACTOR_STEP_STEP_HPP_

#include "labor/Worker.hpp"
#include "StepModel.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Step: public StepModel
{
public:
    Step(Actor::ACTOR_TYPE eActorType, Step* pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout)
        : StepModel(eActorType, pNextStep, dTimeout)
    {
    }
    Step(const Step&) = delete;
    Step& operator=(const Step&) = delete;
    virtual ~Step(){}

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。
     */
    virtual E_CMD_STATUS Emit(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = nullptr) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Cmd> MakeSharedCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Module> MakeSharedModule(const std::string& strModuleName, Targs... args);

private:
    friend class WorkerImpl;
};

template <typename ...Targs>
void Step::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Step::MakeSharedStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Step::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Step::MakeSharedCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Step::MakeSharedModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strModuleName, std::forward<Targs>(args)...));
}


} /* namespace neb */

#endif /* SRC_ACTOR_STEP_STEP_HPP_ */
