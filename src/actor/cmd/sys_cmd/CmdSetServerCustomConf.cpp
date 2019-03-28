/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetServerCustomConf.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdSetServerCustomConf.hpp"

namespace neb
{

CmdSetServerCustomConf::CmdSetServerCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdSetServerCustomConf::~CmdSetServerCustomConf()
{
}

bool CmdSetServerCustomConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    ConfigInfo oConfigInfo;
    if (oConfigInfo.ParseFromString(oInMsgBody.data()))
    {
        CJsonObject oConf;
        if (oConf.Parse(oConfigInfo.file_content()))
        {
            return(GetWorkerImpl(this)->SetCustomConf(oConf));
        }
    }
    return(false);
}

} /* namespace neb */
