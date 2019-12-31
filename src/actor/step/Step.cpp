/*******************************************************************************
 * Project:  Nebula
 * @file     Step.cpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include "Step.hpp"

namespace neb
{

Step::Step(Actor::ACTOR_TYPE eActorType, std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Actor(eActorType, dTimeout),
      m_uiChainId(0)
{
    if (nullptr != pNextStep)
    {
        m_setNextStepSeq.insert(pNextStep->GetSequence());
    }
}

Step::~Step()
{
    m_setNextStepSeq.clear();
    m_setPreStepSeq.clear();
}

void Step::NextStep(int iErrno, const std::string& strErrMsg, void* data)
{
    if (iErrno != ERR_OK)
    {
        return;
    }

    for (auto it = m_setNextStepSeq.begin(); it != m_setNextStepSeq.end(); ++it)
    {
        ExecStep(*it, iErrno, strErrMsg, data);
    }
}

void Step::SetChainId(uint32 uiChainId)
{
    m_uiChainId = uiChainId;
}

} /* namespace neb */
