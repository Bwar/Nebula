/*******************************************************************************
 * Project:  Nebula
 * @file     ChainModel.cpp
 * @brief 
 * @author   bwar
 * @date:    June 7, 2019
 * @note
 * Modify history:
 ******************************************************************************/
#include "ChainModel.hpp"

namespace neb
{

ChainModel::ChainModel(ev_tstamp dChainTimeout)
    : Actor(Actor::ACT_CHAIN, dChainTimeout)
{
}

ChainModel::~ChainModel()
{
}

} /* namespace neb */
