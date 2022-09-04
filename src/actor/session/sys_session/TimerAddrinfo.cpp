/*******************************************************************************
 * Project:  Nebula
 * @file     TimerAddrinfo.cpp
 * @brief    
 * @author   Bwar
 * @date:    2022-08-27
 * @note
 * Modify history:
 ******************************************************************************/
#include "TimerAddrinfo.hpp"

namespace neb
{

TimerAddrinfo::TimerAddrinfo(const std::string& strSessionId)
    : Timer(strSessionId, 10.0), m_pAddrInfo(nullptr)
{
}

TimerAddrinfo::~TimerAddrinfo()
{
    if (m_pAddrInfo != nullptr)
    {
        freeaddrinfo(m_pAddrInfo);
    }
    m_pAddrInfo = nullptr;
}

E_CMD_STATUS TimerAddrinfo::Timeout()
{
    if (m_pAddrInfo != nullptr)
    {
        freeaddrinfo(m_pAddrInfo);
    }
    m_pAddrInfo = nullptr;
    return(CMD_STATUS_RUNNING);
}

void TimerAddrinfo::MigrateAddrinfo(struct addrinfo* pAddrInfo)
{
    if (m_pAddrInfo != nullptr)
    {
        freeaddrinfo(m_pAddrInfo);
    }
    m_pAddrInfo = pAddrInfo;
}

} /* namespace neb */

