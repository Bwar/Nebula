/*******************************************************************************
 * Project:  Nebula
 * @file     CmdReloadCustomConf.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdReloadCustomConf.hpp"

namespace neb
{

CmdReloadCustomConf::CmdReloadCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdReloadCustomConf::~CmdReloadCustomConf()
{
}

bool CmdReloadCustomConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    return(GetWorkerImpl(this)->ReloadCmdConf());
}

} /* namespace neb */
