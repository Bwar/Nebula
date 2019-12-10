/*******************************************************************************
 * Project:  Nebula
 * @file     CmdNodeNotice.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDNODENOTICE_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDNODENOTICE_HPP_

#include "actor/ActorSys.hpp"
#include "actor/cmd/Cmd.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

/**
 * @brief   节点注册
 * @author  hsc
 * @date    2015年8月9日
 * @note    各个模块启动时需要向BEACON进行注册
 */
class CmdNodeNotice : public Cmd,
    public DynamicCreator<CmdNodeNotice, int32>, public ActorSys
{
public:
    CmdNodeNotice(int32 iCmd);
    virtual ~CmdNodeNotice();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDNODENOTICE_HPP_ */
