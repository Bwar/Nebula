/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSetNodeConf.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年3月24日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/cmd/sys_cmd/CmdSetNodeConf.hpp"

namespace neb
{

CmdSetNodeConf::CmdSetNodeConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdSetNodeConf::~CmdSetNodeConf()
{
}

bool CmdSetNodeConf::AnyMessage(
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
            GetLabor(this)->SetNodeConf(oConf);
            return(true);
        }
    }
    return(false);
}

} /* namespace neb */
