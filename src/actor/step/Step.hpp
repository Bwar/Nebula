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

#include <unordered_set>
#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Step: public Actor
{
public:
    Step(ACTOR_TYPE eObjectType, Step* pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout)
        : Actor(eObjectType, dTimeout)
    {
        if (nullptr != pNextStep)
        {
            m_setNextStepSeq.insert(pNextStep->GetSequence());
        }
    }
    Step(const Step&) = delete;
    Step& operator=(const Step&) = delete;
    virtual ~Step();

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。
     */
    template<typename ...Targs>
    virtual E_CMD_STATUS Emit(Targs... args) = 0;

    template<typename ...Targs>
    virtual E_CMD_STATUS Callback(Targs... args) = 0;

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

#endif /* SRC_ACTOR_STEP_STEP_HPP_ */
