/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnSetCustomConf.hpp
 * @brief    设置自定义配置文件
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnSetCustomConf.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnSetCustomConf::CmdOnSetCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnSetCustomConf::~CmdOnSetCustomConf()
{
}

bool CmdOnSetCustomConf::AnyMessage(
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
        std::stringstream ssConfFile;
        if (oConfigInfo.file_path().size() > 0)
        {
            ssConfFile << GetLabor(this)->GetNodeInfo().strWorkPath
                << "/conf/" << oConfigInfo.file_path()
                << "/" << oConfigInfo.file_name();
        }
        else
        {
            ssConfFile << GetLabor(this)->GetNodeInfo().strWorkPath
                << "/conf/" << oConfigInfo.file_name();
        }

        std::ofstream fout(ssConfFile.str().c_str());
        if (fout.good())
        {
            fout.write(oConfigInfo.file_content().c_str(), oConfigInfo.file_content().size());
            fout.close();
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
            oOutMsgBody.mutable_rsp_result()->set_msg("success");
            m_pSessionManager->SendToChild(CMD_REQ_SET_CUSTOM_CONFIG, GetSequence(), oInMsgBody);
            m_pSessionManager->SendToChild(CMD_REQ_RELOAD_CUSTOM_CONFIG, GetSequence(), oInMsgBody);
            SendTo(pChannel, oInMsgHead.cmd() + 1, oInMsgHead.seq(), oOutMsgBody);
            return(true);
        }
        else
        {
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_FILE_NOT_EXIST);
            oOutMsgBody.mutable_rsp_result()->set_msg("file \"" + ssConfFile.str() + "\" not exist!");
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
