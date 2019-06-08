/*******************************************************************************
 * Project:  Nebula
 * @file     ChainModel.hpp
 * @brief 
 * @author   bwar
 * @date:    June 6, 2019
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CHAIN_CHAINMODEL_HPP_
#define SRC_ACTOR_CHAIN_CHAINMODEL_HPP_

#include "actor/Actor.hpp"

namespace neb
{

class ChainModel: public Actor
{
public:
    ChainModel(ev_tstamp dChainTimeout = 60.0);
    ChainModel(const ChainModel&) = delete;
    ChainModel& operator=(const ChainModel&) = delete;
    virtual ~ChainModel();

    /**
     * @brief 功能链超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

private:
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CHAIN_CHAINMODEL_HPP_ */
