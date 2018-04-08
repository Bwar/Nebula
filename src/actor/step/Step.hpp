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
    template <typename ...Targs> void Logger(int iLogLevel, Targs... args);
    template <typename ...Targs> Step* NewStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> Session* NewSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> Cmd* NewCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> Module* NewModule(const std::string& strModuleName, Targs... args);

private:
    friend class WorkerImpl;
};

template <typename ...Targs>
void Step::Logger(int iLogLevel, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, std::forward<Targs>(args)...);
}

template <typename ...Targs>
Step* Step::NewStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->NewStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Session* Step::NewSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->NewSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Cmd* Step::NewCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->NewStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Module* Step::NewModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->NewSession(this, strModuleName, std::forward<Targs>(args)...));
}


} /* namespace neb */

#endif /* SRC_ACTOR_STEP_STEP_HPP_ */
