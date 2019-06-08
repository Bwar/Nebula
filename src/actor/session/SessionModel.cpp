/*******************************************************************************
 * Project:  Nebula
 * @file     SessionModel.cpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include <sstream>
#include "SessionModel.hpp"

namespace neb
{

SessionModel::SessionModel(uint32 ulSessionId, ev_tstamp dSessionTimeout)
    : Actor(Actor::ACT_SESSION, dSessionTimeout)
{
    std::ostringstream oss;
    oss << ulSessionId;
    m_strSessionId = std::move(oss.str());
}

SessionModel::SessionModel(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_strSessionId(strSessionId)
{
}

SessionModel::~SessionModel()
{
}

} /* namespace neb */
