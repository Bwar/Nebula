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

SessionModel::SessionModel(uint32 ulSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_strSessionClass(strSessionClass)
{
    std::ostringstream oss;
    oss << ulSessionId;
    m_strSessionId = std::move(oss.str());
}

SessionModel::SessionModel(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : Actor(Actor::ACT_SESSION, dSessionTimeout),
      m_strSessionId(strSessionId), m_strSessionClass(strSessionClass)
{
}

SessionModel::~SessionModel()
{
    // TODO Auto-generated destructor stub
}

} /* namespace neb */
