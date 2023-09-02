/*******************************************************************************
 * Project:  Nebula
 * @file     DeliverCmd.hpp
 * @brief    线程间数据包传送
 * @author   Bwar
 * @date:    2023-02-11
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_DELIVERCMD_HPP_
#define SRC_ACTOR_CMD_DELIVERCMD_HPP_

#include "Cmd.hpp"
#include "type/Package.hpp"

namespace neb
{

/**
 * @brief 数据包直传
 */
class DeliverCmd: public Cmd
{
public:
    DeliverCmd(int32 iCmd)
        : Cmd(iCmd)
    {
    }
    DeliverCmd(const DeliverCmd&) = delete;
    DeliverCmd& operator=(const DeliverCmd&) = delete;
    virtual ~DeliverCmd(){};

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
            Package&& oPackage) = 0;
};

}

#endif /* SRC_ACTOR_CMD_DELIVERCMD_HPP_ */

