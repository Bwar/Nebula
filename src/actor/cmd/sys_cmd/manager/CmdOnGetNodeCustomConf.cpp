/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnGetNodeCustomConf.cpp
 * @brief    获取节点自定义配置
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnGetNodeCustomConf.hpp"
#include "labor/NodeInfo.hpp"

namespace neb
{

CmdOnGetNodeCustomConf::CmdOnGetNodeCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnGetNodeCustomConf::~CmdOnGetNodeCustomConf()
{
}

bool CmdOnGetNodeCustomConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    MsgBody oOutMsgBody;
    ConfigInfo oConfigInfo;
    CJsonObject oCurrentConf = GetLabor(this)->GetNodeConf();
    oConfigInfo.set_file_name(GetLabor(this)->GetNodeInfo().strConfFile);
    oConfigInfo.set_file_content(oCurrentConf["custom"].ToFormattedString());
    //oOutMsgBody.set_data(oConfigInfo.SerializeAsString());
    oConfigInfo.SerializeToString(&m_strDataString);
    oOutMsgBody.set_data(m_strDataString);
    oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
    oOutMsgBody.mutable_rsp_result()->set_msg("success");
    SendTo(pChannel, oInMsgHead.cmd(), oInMsgHead.seq(), oOutMsgBody);
    return(true);
}

} /* namespace neb */
