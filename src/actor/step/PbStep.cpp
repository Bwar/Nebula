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

PbStep::PbStep(std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, pNextStep, dTimeout)
{
}

PbStep::PbStep(std::shared_ptr<SocketChannel> pChannel, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, pNextStep, dTimeout),
      m_pUpstreamChannel(pChannel), m_oReqMsgHead(oReqMsgHead), m_oReqMsgBody(oReqMsgBody)
{
}

PbStep::~PbStep()
{
}

} /* namespace neb */
