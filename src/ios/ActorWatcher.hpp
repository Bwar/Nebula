/*******************************************************************************
 * Project:  Nebula
 * @file     ActorWatcher.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-03
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_ACTORWATCHER_HPP_
#define SRC_IOS_ACTORWATCHER_HPP_

#include <memory>

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

class Actor;

class ActorWatcher
{
public:
    ActorWatcher();
    ActorWatcher(std::shared_ptr<Actor> pActor);
    virtual ~ActorWatcher();

    ev_timer* MutableTimerWatcher();

    inline std::shared_ptr<Actor> GetActor() const
    {
        return(m_pActor);
    }

    void Set(std::shared_ptr<Actor> pActor);
    void Reset();

private:
    ev_timer* m_pTimerWatcher;
    std::shared_ptr<Actor> m_pActor;
};

} /* namespace neb */

#endif /* SRC_IOS_ACTORWATCHER_HPP_ */

