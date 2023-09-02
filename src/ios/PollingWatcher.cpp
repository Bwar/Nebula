/*******************************************************************************
 * Project:  Nebula
 * @file     PollingWatcher.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-08-05
 * @note     
 * Modify history:
 ******************************************************************************/
#include "PollingWatcher.hpp"

namespace neb
{

PollingWatcher::PollingWatcher()
    : m_uiLaborId(0), m_pDispatcher(nullptr),
      m_pPrepareWatcher(nullptr), m_pCheckWatcher(nullptr), m_pTimerWatcher(nullptr)
{
}

PollingWatcher::PollingWatcher(Dispatcher* pDispatcher, uint32 uiLaborId)
    : m_uiLaborId(uiLaborId), m_pDispatcher(pDispatcher),
      m_pPrepareWatcher(nullptr), m_pCheckWatcher(nullptr), m_pTimerWatcher(nullptr)
{
}

PollingWatcher::~PollingWatcher()
{
    Reset();
}

ev_prepare* PollingWatcher::MutablePrepareWatcher()
{
    if (nullptr == m_pDispatcher)
    {
        return(nullptr);
    }
    if (nullptr == m_pPrepareWatcher)
    {
        m_pPrepareWatcher = new ev_prepare();
        if (nullptr != m_pPrepareWatcher)
        {
            memset(m_pPrepareWatcher, 0, sizeof(ev_prepare));
            m_pPrepareWatcher->data = this;
        }
    }
    return(m_pPrepareWatcher);
}

ev_check* PollingWatcher::MutableCheckWatcher()
{
    if (nullptr == m_pDispatcher)
    {
        return(nullptr);
    }
    if (nullptr == m_pCheckWatcher)
    {
        m_pCheckWatcher = new ev_check();
        if (nullptr != m_pCheckWatcher)
        {
            memset(m_pCheckWatcher, 0, sizeof(ev_check));
            m_pCheckWatcher->data = this;
        }
    }
    return(m_pCheckWatcher);
}

ev_timer* PollingWatcher::MutableTimerWatcher()
{
    if (nullptr == m_pDispatcher)
    {
        return(nullptr);
    }
    if (nullptr == m_pTimerWatcher)
    {
        m_pTimerWatcher = new ev_timer();
        if (nullptr != m_pTimerWatcher)
        {
            memset(m_pTimerWatcher, 0, sizeof(ev_timer));
            m_pTimerWatcher->data = this;
        }
    }
    return(m_pTimerWatcher);
}

void PollingWatcher::Set(Dispatcher* pDispatcher, uint32 uiLaborId)
{
    if (m_pDispatcher == nullptr)
    {
        m_pDispatcher = pDispatcher;
    }
    m_uiLaborId = uiLaborId;
}

void PollingWatcher::Reset()
{
    m_pDispatcher = nullptr;
    if (nullptr != m_pTimerWatcher)
    {
        delete m_pTimerWatcher;
        m_pTimerWatcher = nullptr;
    }
    if (nullptr != m_pPrepareWatcher)
    {
        delete m_pPrepareWatcher;
        m_pPrepareWatcher = nullptr;
    }
    if (nullptr != m_pCheckWatcher)
    {
        delete m_pCheckWatcher;
        m_pCheckWatcher = nullptr;
    }
}

} /* namespace neb */

