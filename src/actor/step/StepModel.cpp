/*******************************************************************************
 * Project:  Nebula
 * @file     StepModel.cpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepModel.hpp"
#include "Step.hpp"

namespace neb
{

StepModel::StepModel(Actor::ACTOR_TYPE eActorType, std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Actor(eActorType, dTimeout)
{
    if (nullptr != pNextStep)
    {
        m_setNextStepSeq.insert(pNextStep->GetSequence());
    }
}

StepModel::~StepModel()
{
    m_setNextStepSeq.clear();
    m_setPreStepSeq.clear();
}

void StepModel::NextStep(int iErrno, const std::string& strErrMsg, void* data)
{
    if (iErrno != ERR_OK)
    {
        return;
    }

    for (auto it = m_setNextStepSeq.begin(); it != m_setNextStepSeq.end(); ++it)
    {
        ExecStep(*it);
    }
}

} /* namespace neb */
