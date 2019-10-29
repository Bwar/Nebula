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
#include "actor/ActorBuilder.hpp"

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
    return(GetLabor(this)->GetActorBuilder()->ReloadCmdConf());
}

} /* namespace neb */
