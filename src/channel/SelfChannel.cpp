/*******************************************************************************
 * Project:  Nebula
 * @file     SelfChannel.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include "SelfChannel.hpp"

namespace neb
{

SelfChannel::SelfChannel(uint32 uiSeq)
    : m_bIsResponse(false), m_uiChannelSeq(uiSeq), m_uiStepSeq(0)
{
}

SelfChannel::~SelfChannel()
{
}

}
