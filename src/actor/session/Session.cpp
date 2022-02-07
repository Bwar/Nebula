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
    : Actor(Actor::ACT_SESSION, dSessionTimeout)
{
    std::ostringstream oss;
    oss << uiSessionId;
    m_strSessionId = std::move(oss.str());
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_strSessionId(strSessionId)
{
}

Session::Session(ACTOR_TYPE eActorType, uint32 ulSessionId, ev_tstamp dSessionTimeout)
    : Actor(eActorType, dSessionTimeout)
{
    std::ostringstream oss;
    oss << ulSessionId;
    m_strSessionId = std::move(oss.str());
}

Session::Session(ACTOR_TYPE eActorType, const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Actor(eActorType, dSessionTimeout),
      m_strSessionId(strSessionId)
{
}

Session::~Session()
{
}

} /* namespace neb */
