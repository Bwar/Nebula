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

#include <memory>
#include <list>
#include <string>
#include "hiredis/async.h"
#include "Channel.hpp"
#include "Definition.hpp"

namespace neb
{

class Dispatcher;
class ActorBuilder;
class RedisStep;

class RedisChannel: public Channel
{
public:
    RedisChannel(redisAsyncContext *c);
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
    std::list< std::shared_ptr<RedisStep> > listPipelineStep; ///< RedisStep*的创建和回收在WorkerImpl
    friend Dispatcher;
    friend ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_REDISCHANNEL_HPP_ */
