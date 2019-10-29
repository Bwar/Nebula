/*******************************************************************************
 * Project:  Nebula
 * @file     Session.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Session.hpp"
#include "actor/step/Step.hpp"

namespace neb
{

Session::Session(uint32 uiSessionId, ev_tstamp dSessionTimeout)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_bDataReady(false), m_bDataLoading(false)
{
    std::ostringstream oss;
    oss << uiSessionId;
    m_strSessionId = std::move(oss.str());
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_bDataReady(false), m_bDataLoading(false),
      m_strSessionId(strSessionId)
{
}

Session::Session(ACTOR_TYPE eActorType, uint32 ulSessionId, ev_tstamp dSessionTimeout)
    : Actor(eActorType, dSessionTimeout),
      m_bDataReady(false), m_bDataLoading(false)
{
    std::ostringstream oss;
    oss << ulSessionId;
    m_strSessionId = std::move(oss.str());
}

Session::Session(ACTOR_TYPE eActorType, const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Actor(eActorType, dSessionTimeout),
      m_bDataReady(false), m_bDataLoading(false),
      m_strSessionId(strSessionId)
{
}

Session::~Session()
{
}

bool Session::IsReady(Step* pStep)
{
    if (!m_bDataReady)
    {
        m_vecWaitingStep.push(pStep->GetSequence());
    }
    return(m_bDataReady);
}

void Session::SetReady()
{
    m_bDataReady = true;
    m_bDataLoading = false;
    if (m_vecWaitingStep.size() > 0)
    {
        AddAssemblyLine(std::dynamic_pointer_cast<Session>(shared_from_this()));
    }
}

bool Session::IsLoading()
{
    return(m_bDataLoading);
}

void Session::SetLoading()
{
    m_bDataLoading = true;
    m_bDataReady = false;
}

uint32 Session::PopWaitingStep()
{
    uint32 uiStepSeq = 0;
    if (m_vecWaitingStep.size() > 0)
    {
        uiStepSeq = m_vecWaitingStep.front();
        m_vecWaitingStep.pop();
    }
    return(uiStepSeq);
}

} /* namespace neb */
