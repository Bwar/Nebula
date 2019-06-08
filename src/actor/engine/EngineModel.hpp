/*******************************************************************************
 * Project:  Nebula
 * @file     EngineModel.hpp
 * @brief 
 * @author   bwar
 * @date:    June 6, 2019
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_ENGINEMODEL_HPP_
#define SRC_ACTOR_ENGINEMODEL_HPP_

#include "actor/Actor.hpp"

namespace neb
{

class EngineModel: public Actor
{
public:
    EngineModel(Actor::ACTOR_TYPE eActorType, ev_tstamp dTimeout = gc_dDefaultTimeout)
        : Actor(eActorType, dTimeout)
    {
    }
    EngineModel(const EngineModel&) = delete;
    EngineModel& operator=(const EngineModel&) = delete;
    virtual ~EngineModel();

    /**
     * @brief 提交
     */
    virtual E_CMD_STATUS Launch() = 0;

private:
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_ENGINEMODEL_HPP_ */
