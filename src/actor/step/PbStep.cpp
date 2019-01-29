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

PbStep::PbStep(std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, pNextStep, dTimeout)
{
}

PbStep::PbStep(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, pNextStep, dTimeout),
      m_pChannel(pChannel), m_iReqCmd(iCmd), m_uiReqSeq(uiSeq)
{
}

PbStep::PbStep(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oReqMsgBody, std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(Actor::ACT_PB_STEP, pNextStep, dTimeout),
      m_pChannel(pChannel), m_iReqCmd(iCmd), m_uiReqSeq(uiSeq), m_oReqMsgBody(oReqMsgBody)
{
}

PbStep::~PbStep()
{
}

} /* namespace neb */
