/*******************************************************************************
 * Project:  Nebula
 * @file     CmdUpdateNodeId.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_CMDUPDATENODEID_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_CMDUPDATENODEID_HPP_

#include "object/cmd/Cmd.hpp"

namespace neb
{

class CmdUpdateNodeId: public Cmd
{
public:
    CmdUpdateNodeId();
    virtual ~CmdUpdateNodeId();
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);

    virtual std::string ObjectName() const
    {
        return("CmdUpdateNodeId");
    }
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_CMDUPDATENODEID_HPP_ */
