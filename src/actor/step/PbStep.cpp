/*******************************************************************************
 * Project:  Nebula
 * @file     Step.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/step/PbStep.hpp>

namespace neb
{

PbStep::PbStep(ev_tstamp dTimeout, Step* pNextStep)
    : Step(ACT_PB_STEP, dTimeout, pNextStep)
{
}

PbStep::PbStep(const tagChannelContext& stCtx, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, ev_tstamp dTimeout, Step* pNextStep)
    : Step(ACT_PB_STEP, dTimeout, pNextStep),
      m_stCtx(stCtx), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody)
{
}

PbStep::~PbStep()
{
}

} /* namespace neb */
