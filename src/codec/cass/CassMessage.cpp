/*******************************************************************************
 * Project:  Nebula
 * @file     CassMessage.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#include "CassMessage.hpp"

namespace neb
{

CassMessage::CassMessage(uint16 uiMsgType)
    : m_unMsgType(uiMsgType), m_unStreamId(0)
{
}

CassMessage::~CassMessage()
{
}

} /* namespace neb */

