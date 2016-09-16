/*******************************************************************************
 * Project:  Nebula
 * @file     CmdConnectWorker.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_
#define SRC_OBJECT_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_

#include "pb/neb_sys.pb.h"
#include "object/cmd/Cmd.hpp"
#include "object/step/sys_step/StepConnectWorker.hpp"

namespace neb
{

class CmdConnectWorker: public Cmd
{
public:
    CmdConnectWorker();
    virtual ~CmdConnectWorker();
    virtual bool AnyMessage(
            const tagChannelContext& stCtx,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody);
    bool Start(const tagChannelContext& stCtx, int iWorkerIndex);

    virtual std::string ObjectName() const
    {
        return("CmdConnectWorker");
    }

public:
    StepConnectWorker* pStepConnectWorker;        ///< 仅为了生成可读性高的类图，构造函数中不分配空间，析构函数中也不回收空间
};

} /* namespace neb */

#endif /* SRC_OBJECT_CMD_SYS_CMD_CMDCONNECTWORKER_HPP_ */
