/*******************************************************************************
 * Project:  Nebula
 * @file     Timer.cpp
 * @brief 
 * @author   Bwar
 * @date:    2018年8月5日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Timer.hpp"

namespace neb
{

Timer::Timer(uint32 ulSessionId, ev_tstamp dSessionTimeout)
    : Session(ACT_TIMER, ulSessionId, dSessionTimeout)
{
}

Timer::Timer(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Session(ACT_TIMER, strSessionId, dSessionTimeout)
{
}

Timer::~Timer()
{
}

} /* namespace neb */
