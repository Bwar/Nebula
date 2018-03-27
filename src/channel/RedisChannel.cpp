/*******************************************************************************
 * Project:  Nebula
 * @file     RedisChannel.cpp
 * @brief 
 * @author   bwar
 * @date:    Mar 25, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisChannel.hpp"

namespace neb
{

RedisChannel::RedisChannel()
    : bIsReady(false), m_pRedisCtx(nullptr)
{
    
}

RedisChannel::~RedisChannel()
{
    if (nullptr != m_pRedisCtx)
    {
        redisAsyncFree(m_pRedisCtx);
    }
}

} /* namespace neb */
