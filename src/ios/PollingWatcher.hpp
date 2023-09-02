/*******************************************************************************
 * Project:  Nebula
 * @file     PollingWatcher.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-08-05
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_POLLINGWATCHER_HPP_
#define SRC_IOS_POLLINGWATCHER_HPP_

#include <memory>
#include "Definition.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include "ev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

namespace neb
{

class Dispatcher;

class PollingWatcher
{
public:
    PollingWatcher();
    PollingWatcher(Dispatcher* pDispatcher, uint32 uiLaborId);
    virtual ~PollingWatcher();

    ev_prepare* MutablePrepareWatcher();
    ev_check* MutableCheckWatcher();
    ev_timer* MutableTimerWatcher();

    inline Dispatcher* GetDispatcher() const
    {
        return(m_pDispatcher);
    }

    uint32 GetLaborId() const
    {
        return(m_uiLaborId);
    }

    void Set(Dispatcher* pDispatcher, uint32 uiLaborId);
    void Reset();

private:
    uint32 m_uiLaborId;
    Dispatcher* m_pDispatcher;
    ev_prepare* m_pPrepareWatcher;
    ev_check* m_pCheckWatcher;
    ev_timer* m_pTimerWatcher;
};

} /* namespace neb */

#endif /* SRC_IOS_POLLINGWATCHER_HPP_ */

