/*******************************************************************************
 * Project:  Nebula
 * @file     RedisCmd.hpp
 * @brief    redis请求处理入口
 * @author   Bwar
 * @date:    2020年12月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_REDISCMD_HPP_
#define SRC_ACTOR_CMD_REDISCMD_HPP_

#include "Cmd.hpp"

namespace neb
{

/**
 * @brief redis请求处理入口
 * @note 使用Nebula框架开发redis数据代理或开发基于redis协议的存储服务时会用到RedisCmd,
 * 其他场景不会用到。使用RedisCmd在加载插件时，配置文件里的cmd应配置成CMD_REQ_REDIS_PROXY
 * 值为403，否则框架不能正确调用该插件。
 */
class RedisCmd: public Cmd
{
public:
    RedisCmd(int32 iCmd)
        : Cmd(iCmd)
    {
    }
    RedisCmd(const RedisCmd&) = delete;
    RedisCmd& operator=(const RedisCmd&) = delete;
    virtual ~RedisCmd(){};

    virtual bool Init()
    {
        return(true);
    }

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oMsgHead, const MsgBody& oMsgBody)
    {
        return(false);
    }

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const RedisMsg& oRedisMsg) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_REDISCMD_HPP_ */
