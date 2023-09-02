/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnSetNodeCustomConf.cpp
 * @brief    设置节点自定义信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnSetNodeCustomConf.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnSetNodeCustomConf::CmdOnSetNodeCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnSetNodeCustomConf::~CmdOnSetNodeCustomConf()
{
}

bool CmdOnSetNodeCustomConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    if (m_pSessionManager == nullptr)
    {
        m_pSessionManager = std::dynamic_pointer_cast<SessionManager>(
                GetSession("neb::SessionManager"));
        if (m_pSessionManager == nullptr)
        {
            LOG4_ERROR("no session named \"neb::SessionManager\"!");
            return(false);
        }
    }
    MsgBody oOutMsgBody;
    ConfigInfo oConfigInfo;
    if (oConfigInfo.ParseFromString(oInMsgBody.data()))
    {
        CJsonObject oJsonData;
        if (oJsonData.Parse(oConfigInfo.file_content()))
        {
            CJsonObject oCurrentConf = GetLabor(this)->GetNodeConf();
            oCurrentConf.Replace("custom", oJsonData);
            std::ofstream fout(GetLabor(this)->GetNodeInfo().strConfFile.c_str());
            if (fout.good())
            {
                std::string strNewConfData = oCurrentConf.ToFormattedString();
                fout.write(strNewConfData.c_str(), strNewConfData.size());
                fout.close();
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
                oOutMsgBody.mutable_rsp_result()->set_msg("success");
                m_pSessionManager->SendToChild(CMD_REQ_SET_NODE_CUSTOM_CONFIG, GetSequence(), oInMsgBody);
                GetLabor(this)->SetNodeConf(oCurrentConf);
                SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
                return(true);
            }
            else
            {
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_FILE_NOT_EXIST);
                oOutMsgBody.mutable_rsp_result()->set_msg("failed to open the server config file!");
                SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
                return(false);
            }
        }
        else
        {
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_BODY_JSON);
            oOutMsgBody.mutable_rsp_result()->set_msg("the server custom config must be in json format!");
            SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
            return(false);
        }
    }
    else
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
        oOutMsgBody.mutable_rsp_result()->set_msg("ConfigInfo.ParseFromString() failed.");
        SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
        return(false);
    }
}

} /* namespace neb */
