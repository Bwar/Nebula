/*******************************************************************************
 * Project:  Nebula
 * @file     CmdDataReport.hpp
 * @brief    心跳包响应
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef ACTOR_CMD_SYS_CMD_CMDDATAREPORT_HPP_
#define ACTOR_CMD_SYS_CMD_CMDDATAREPORT_HPP_

#include "actor/cmd/Cmd.hpp"

namespace neb
{

class SessionDataReport;

class CmdDataReport: public Cmd,
    public DynamicCreator<CmdDataReport, int32>
{
public:
    CmdDataReport(int32 iCmd);
    virtual ~CmdDataReport();
    virtual bool Init();
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oInMsgHead,
                    const MsgBody& oInMsgBody);
private:
    std::shared_ptr<SessionDataReport> pSessionDataReport = nullptr;
};

} /* namespace neb */

#endif /* ACTOR_CMD_SYS_CMD_CMDDATAREPORT_HPP_ */
