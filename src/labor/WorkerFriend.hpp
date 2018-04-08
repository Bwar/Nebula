/*******************************************************************************
* Project:  Nebula
* @file     ActorSys.hpp
* @brief 
* @author   bwar
* @date:    Mar 21, 2018
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_ACTOR_WORKERFRIEND_HPP_
#define SRC_ACTOR_WORKERFRIEND_HPP_

#include "Worker.hpp"
#include "actor/Actor.hpp"

namespace neb
{

class WorkerFriend
{
public:
    WorkerImpl* GetWorkerImpl(Actor* pActor)
    {
        return(pActor->m_pWorker->m_pImpl);
    }
};

}

#endif /* SRC_ACTOR_WORKERFRIEND_HPP_ */
