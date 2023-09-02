/*******************************************************************************
 * Project:  Nebula
 * @file     DeliverStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2023-02-11
 * @note
 * Modify history:
 ******************************************************************************/
#include "DeliverStep.hpp"

namespace neb
{

DeliverStep::DeliverStep(ev_tstamp dTimeout)
    : Step(ACT_REDIS_STEP, dTimeout)
{
}

DeliverStep::~DeliverStep()
{
}

} /* namespace neb */

