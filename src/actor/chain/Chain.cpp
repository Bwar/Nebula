/*******************************************************************************
 * Project:  Nebula
 * @file     Chain.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月7日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Chain.hpp"
#include "actor/step/Step.hpp"

namespace neb
{

Chain::Chain(ev_tstamp dChainTimeout)
    : ChainModel(dChainTimeout)
{
}

Chain::~Chain()
{
}

} /* namespace neb */
