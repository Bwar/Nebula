/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnGetNodeConf.cpp
 * @brief    获取节点配置
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnGetNodeConf.hpp"
#include "labor/NodeInfo.hpp"

namespace neb
{

CmdOnGetNodeConf::CmdOnGetNodeConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnGetNodeConf::~CmdOnGetNodeConf()
{
}

bool CmdOnGetNodeConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    MsgBody oOutMsgBody;
    ConfigInfo oConfigInfo;
    oConfigInfo.set_file_name(GetLabor(this)->GetNodeInfo().strConfFile);
    oConfigInfo.set_file_content(GetLabor(this)->GetNodeConf().ToFormattedString());
    //oOutMsgBody.set_data(oConfigInfo.SerializeAsString());
    oConfigInfo.SerializeToString(&m_strDataString);
    oOutMsgBody.set_data(m_strDataString);
    oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
    oOutMsgBody.mutable_rsp_result()->set_msg("success");
    SendTo(pChannel, oInMsgHead.cmd(), oInMsgHead.seq(), oOutMsgBody);
    return(true);
}

} /* namespace neb */
