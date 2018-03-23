/*******************************************************************************
 * Project:  Nebula
 * @file     Step.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/step/Step.hpp>

namespace neb
{

Step::Step(ACTOR_TYPE eObjectType, ev_tstamp dTimeout, Step* pNextStep)
    : Actor(eObjectType, dTimeout)
{
    AddNextStepSeq(pNextStep);
}

Step::~Step()
{
}

void Step::AddNextStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered())
    {
        m_setNextStepSeq.insert(pStep->GetSequence());
    }
}

void Step::AddPreStepSeq(Step* pStep)
{
    if (NULL != pStep && pStep->IsRegistered())
    {
        m_setPreStepSeq.insert(pStep->GetSequence());
    }
}

} /* namespace neb */
