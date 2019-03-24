/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetServerConf.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月24日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdSetServerConf.hpp"

namespace neb
{

CmdSetServerConf::CmdSetServerConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdSetServerConf::~CmdSetServerConf()
{
}

bool CmdSetServerConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    CJsonObject oConf;
    if (oConf.Parse(oInMsgBody.data()))
    {
        return(GetWorkerImpl(this)->SetWorkerConf(oConf));
    }
    return(false);
}

} /* namespace neb */
