/*******************************************************************************
 * Project:  Nebula
 * @file     RedisChannel.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 25, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_REDISCHANNEL_HPP_
#define SRC_CHANNEL_REDISCHANNEL_HPP_

#include <list>
#include "Channel.hpp"
#include "Definition.hpp"

namespace neb
{

class RedisChannel: public Channel
{
public:
    RedisChannel();
    virtual ~RedisChannel();

    const std::string& GetIdentify()
    {
        return(m_strIdentify);
    }

    void SetIdentify(const std::string& strIdentify)
    {
        m_strIdentify = strIdentify;
    }

    redisAsyncContext* RedisContext()
    {
        return(m_pRedisCtx);
    }

private:
    bool bIsReady;                          ///< redis连接是否准备就绪
    redisAsyncContext* m_pRedisCtx;         ///< redis连接上下文地址
    std::string m_strIdentify;              ///< 连接标识（可以为空，不为空时用于标识业务层与连接的关系）
    std::list<RedisStep*> listPipelineStep; ///< RedisStep*的创建和回收在WorkerImpl
};

} /* namespace neb */

#endif /* SRC_CHANNEL_REDISCHANNEL_HPP_ */
