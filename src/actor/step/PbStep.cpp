/*******************************************************************************
 * Project:  Nebula
 * @file     PbStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#include "PbStep.hpp"

namespace neb
{

PbStep::PbStep(ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, dTimeout)
{
}

PbStep::~PbStep()
{
}

} /* namespace neb */
