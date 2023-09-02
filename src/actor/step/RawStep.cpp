/*******************************************************************************
 * Project:  Nebula
 * @file     RawStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2020年12月6日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/step/RawStep.hpp"

namespace neb
{

RawStep::RawStep(ev_tstamp dTimeout)
    : Step(ACT_REDIS_STEP, dTimeout)
{
}

RawStep::~RawStep()
{
}

} /* namespace neb */
