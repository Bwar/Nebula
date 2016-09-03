/*******************************************************************************
 * Project:  Nebula
 * @file     CmdBeat.hpp
 * @brief    心跳包响应
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_CMDBEAT_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_CMDBEAT_HPP_

#include "object/cmd/Cmd.hpp"

namespace neb
{

class CmdBeat: public Cmd
{
public:
    CmdBeat();
    virtual ~CmdBeat();
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);

    virtual std::string ObjectName() const
    {
        return("CmdBeat");
    }
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_CMDBEAT_HPP_ */
