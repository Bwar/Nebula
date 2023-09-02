/*******************************************************************************
 * Project:  Nebula
 * @file     Context.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Context.hpp"

namespace neb
{

Context::Context()
    : Actor(ACT_CONTEXT, gc_dNoTimeout),
      m_pChannel(nullptr)
{
}

Context::Context(std::shared_ptr<SocketChannel> pChannel)
    : Actor(ACT_CONTEXT, gc_dNoTimeout),
      m_pChannel(pChannel)
{
}

Context::~Context()
{
}

} /* namespace neb */
