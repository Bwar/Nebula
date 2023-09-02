/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetNodeCustomConf.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月28日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdSetNodeCustomConf.hpp"
#include "labor/Worker.hpp"

namespace neb
{

CmdSetNodeCustomConf::CmdSetNodeCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdSetNodeCustomConf::~CmdSetNodeCustomConf()
{
}

bool CmdSetNodeCustomConf::AnyMessage(
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
            return(((Worker*)GetLabor(this))->SetCustomConf(oConf));
        }
    }
    return(false);
}

} /* namespace neb */
