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

RedisChannel::RedisChannel(redisAsyncContext *c)
    : bIsReady(false), m_pRedisCtx(c)
{
}

RedisChannel::~RedisChannel()
{
    // RedisChannel should not be active closed.
    //if (NULL != m_pRedisCtx)
    //{
    //    redisAsyncFree(m_pRedisCtx);
    //}
}

} /* namespace neb */
