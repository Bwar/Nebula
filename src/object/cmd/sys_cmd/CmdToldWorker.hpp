/*******************************************************************************
 * Project:  Nebula
 * @file     CmdToldWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_CMDTOLDWORKER_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_CMDTOLDWORKER_HPP_

#include "object/cmd/Cmd.hpp"

namespace neb
{

class CmdToldWorker: public Cmd
{
public:
    CmdToldWorker();
    virtual ~CmdToldWorker();
    virtual bool AnyMessage(
                    const tagChannelContext& stCtx,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);

    virtual std::string ObjectName() const
    {
        return("CmdToldWorker");
    }
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_CMDTOLDWORKER_HPP_ */
