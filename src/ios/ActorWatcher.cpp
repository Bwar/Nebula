/*******************************************************************************
 * Project:  Nebula
 * @file     ActorWatcher.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-03
 * @note     
 * Modify history:
 ******************************************************************************/
#include "ActorWatcher.hpp"

namespace neb
{

ActorWatcher::ActorWatcher()
    : m_pTimerWatcher(nullptr), m_pActor(nullptr)
{
}

ActorWatcher::ActorWatcher(std::shared_ptr<Actor> pActor)
    : m_pTimerWatcher(nullptr), m_pActor(pActor)
{
}

ActorWatcher::~ActorWatcher()
{
    Reset();
}

ev_timer* ActorWatcher::MutableTimerWatcher()
{
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

void ActorWatcher::Set(std::shared_ptr<Actor> pActor)
{
    if (m_pActor == nullptr)
    {
        m_pActor = pActor;
    }
}

void ActorWatcher::Reset()
{
    m_pActor = nullptr;
    if (nullptr != m_pTimerWatcher)
    {
        delete m_pTimerWatcher;
        m_pTimerWatcher = nullptr;
    }
}

} /* namespace neb */

