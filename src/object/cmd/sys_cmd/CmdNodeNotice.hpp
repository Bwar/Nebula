/*******************************************************************************
 * Project:  Nebula
 * @file     CmdNodeNotice.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_CMDNODENOTICE_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_CMDNODENOTICE_HPP_

#include "pb/neb_sys.pb.h"
#include "object/cmd/Cmd.hpp"
#include "object/step/sys_step/StepNodeNotice.hpp"

namespace neb
{

/**
 * @brief   节点注册
 * @author  hsc
 * @date    2015年8月9日
 * @note    各个模块启动时需要向CENTER进行注册
 */
class CmdNodeNotice : public Cmd
{
public:
    CmdNodeNotice();
    virtual ~CmdNodeNotice();
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);

    virtual std::string ObjectName() const
    {
        return("CmdNodeNotice");
    }

public:
    StepNodeNotice* pStepNodeNotice;
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_CMDNODENOTICE_HPP_ */
