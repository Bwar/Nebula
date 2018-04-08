/*******************************************************************************
 * Project:  Nebula
 * @file     CmdConnectWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_
#define SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_

#include "actor/cmd/Cmd.hpp"
#include "actor/step/sys_step/StepConnectWorker.hpp"
#include "pb/neb_sys.pb.h"

namespace neb
{

class CmdConnectWorker: public Cmd, public DynamicCreator<CmdConnectWorker, int>
{
public:
    CmdConnectWorker(int iCmd);
    virtual ~CmdConnectWorker();
    virtual bool AnyMessage(
            const tagChannelContext& stCtx,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
    bool Start(const tagChannelContext& stCtx, int iWorkerIndex);

public:
    StepConnectWorker* pStepConnectWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace neb */

#endif /* SRC_ACTOR_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_ */
