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
    : Object(OBJ_SESSION),
      m_strSessionId("0"), m_strSessionClassName(strSessionClass)
{
    char szSessionId[16] = {0};
    snprintf(szSessionId, sizeof(szSessionId), "%u", ulSessionId);
    m_strSessionId = szSessionId;
    SetTimeout(dSessionTimeout);
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : Object(OBJ_SESSION),
      m_strSessionId(strSessionId), m_strSessionClassName(strSessionClass)
{
    SetTimeout(dSessionTimeout);
}

Session::~Session()
{
}

} /* namespace neb */
