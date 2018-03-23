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

namespace neb
{

Session::Session(uint32 ulSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : Actor(ACT_SESSION),
      m_strSessionId("0"), m_strSessionClassName(strSessionClass)
{
    std::ostringstream oss;
    oss << ulSessionId;
    m_strSessionId = std::move(oss.str());
    SetTimeout(dSessionTimeout);
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : Actor(ACT_SESSION),
      m_strSessionId(strSessionId), m_strSessionClassName(strSessionClass)
{
    SetTimeout(dSessionTimeout);
}

Session::~Session()
{
}

} /* namespace neb */
