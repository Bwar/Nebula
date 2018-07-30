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

Session::Session(uint32 ulSessionId, ev_tstamp dSessionTimeout)
    : SessionModel(ulSessionId, dSessionTimeout)
{
}

Session::Session(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : SessionModel(strSessionId, dSessionTimeout)
{
}

Session::~Session()
{
}

} /* namespace neb */
