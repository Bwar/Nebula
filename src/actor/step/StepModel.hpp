/*******************************************************************************
 * Project:  Nebula
 * @file     StepModel.hpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_STEPMODEL_HPP_
#define SRC_ACTOR_STEP_STEPMODEL_HPP_

#include <unordered_set>
#include "actor/Actor.hpp"

namespace neb
{

class Step;

class StepModel: public Actor
{
public:
    StepModel(Actor::ACTOR_TYPE eActorType, Step* pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout);
    StepModel(const StepModel&) = delete;
    StepModel& operator=(const StepModel&) = delete;
    virtual ~StepModel();

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。
     */
    virtual E_CMD_STATUS Emit(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = nullptr) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

private:
    std::unordered_set<uint32> m_setNextStepSeq;
    std::unordered_set<uint32> m_setPreStepSeq;
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_STEPMODEL_HPP_ */
