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
    // TODO Auto-generated destructor stub
}

} /* namespace neb */
