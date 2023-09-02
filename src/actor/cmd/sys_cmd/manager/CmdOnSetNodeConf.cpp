/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnSetNodeConf.cpp
 * @brief    设置节点信息
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnSetNodeConf.hpp"
#include "util/json/CJsonObject.hpp"
#include "ios/Dispatcher.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnSetNodeConf::CmdOnSetNodeConf(int iCmd)
    : Cmd(iCmd), m_pSessionManager(nullptr)
{
}

CmdOnSetNodeConf::~CmdOnSetNodeConf()
{
}

bool CmdOnSetNodeConf::AnyMessage(
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
            // some data can not be set by beacon.
            CJsonObject oCurrentConf = GetLabor(this)->GetNodeConf();
            oJsonData.Replace("node_type", oCurrentConf("node_type"));
            oJsonData.Replace("access_host", oCurrentConf("access_host"));
            oJsonData.Replace("access_port", oCurrentConf("access_port"));
            oJsonData.Replace("access_codec", oCurrentConf("access_codec"));
            oJsonData.Replace("host", oCurrentConf("host"));
            oJsonData.Replace("port", oCurrentConf("port"));
            oJsonData.Replace("server_name", oCurrentConf("server_name"));
            oJsonData.Replace("worker_num", oCurrentConf("worker_num"));
            std::ofstream fout(GetLabor(this)->GetNodeInfo().strConfFile.c_str());
            if (fout.good())
            {
                std::string strNewConfData = oJsonData.ToFormattedString();
                fout.write(strNewConfData.c_str(), strNewConfData.size());
                fout.close();
                oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
                oOutMsgBody.mutable_rsp_result()->set_msg("success");
                m_pSessionManager->SendToChild(CMD_REQ_SET_NODE_CONFIG, GetSequence(), oInMsgBody);
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
            oOutMsgBody.mutable_rsp_result()->set_msg("the server config must be in json format!");
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
