/*******************************************************************************
 * Project:  Nebula
 * @file     Step.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#include "PbStep.hpp"

namespace neb
{

PbStep::PbStep(Step* pNextStep)
    : Step(OBJ_PB_STEP, pNextStep)
{
}

PbStep::PbStep(const tagChannelContext& stCtx, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep)
    : Step(OBJ_PB_STEP, pNextStep),
      m_stCtx(stCtx), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody)
{
}

PbStep::~PbStep()
{
}

} /* namespace neb */
