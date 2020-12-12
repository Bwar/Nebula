/*******************************************************************************
 * Project:  Nebula
 * @file     RawCmd.hpp
 * @brief    raw请求处理入口
 * @author   Bwar
 * @date:    2020年12月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_RAWCMD_HPP_
#define SRC_ACTOR_CMD_RAWCMD_HPP_

#include "Cmd.hpp"

namespace neb
{

/**
 * @brief raw请求处理入口
 * @note 使用Nebula框架开发自定义服务时会用到RawCmd,
 * 其他场景不会用到。使用RawCmd在加载插件时，配置文件里的cmd应配置成CMD_REQ_RAW_DATA
 * 值为405RedisCmd.，否则框架不能正确调用该插件。
 */
class RawCmd: public Cmd
{
public:
    RawCmd(int32 iCmd)
        : Cmd(iCmd)
    {
    }
    RawCmd(const RawCmd&) = delete;
    RawCmd& operator=(const RawCmd&) = delete;
    virtual ~RawCmd(){};

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
            const char* pRawData, uint32 uiRawDataSize) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_RAWCMD_HPP_ */
