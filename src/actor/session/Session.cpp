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
    : SessionModel(ulSessionId, dSessionTimeout, strSessionClass)
{
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout, const std::string& strSessionClass)
    : SessionModel(strSessionId, dSessionTimeout, strSessionClass)
{
}

Session::~Session()
{
}

} /* namespace neb */
