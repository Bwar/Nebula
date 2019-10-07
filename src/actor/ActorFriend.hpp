/*******************************************************************************
 * Project:  Nebula
 * @file     ActorFriend.hpp
 * @brief    Actor友元类，用于访问Actor的保护或私有成员
 * @author   Bwar
 * @date:    2019年9月21日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_ACTORFRIEND_HPP_
#define SRC_ACTOR_ACTORFRIEND_HPP_

#include "labor/Labor.hpp"
#include "Actor.hpp"

namespace neb
{

class ActorFriend
{
public:
    Labor* GetLabor(Actor* pActor)
    {
        return(pActor->m_pLabor);
    }
};

}

#endif /* SRC_ACTOR_ACTORFRIEND_HPP_ */
