/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnGetCustomConf.cpp
 * @brief    获取节点自定义配置
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnGetCustomConf.hpp"
#include "labor/NodeInfo.hpp"

namespace neb
{

CmdOnGetCustomConf::CmdOnGetCustomConf(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdOnGetCustomConf::~CmdOnGetCustomConf()
{
}

bool CmdOnGetCustomConf::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
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

        std::ifstream fin(ssConfFile.str().c_str());
        if (fin.good())
        {
            std::stringstream ssContent;
            ssContent << fin.rdbuf();
            oConfigInfo.set_file_content(ssContent.str());
            //oOutMsgBody.set_data(oConfigInfo.SerializeAsString());
            oConfigInfo.SerializeToString(&m_strDataString);
            oOutMsgBody.set_data(m_strDataString);
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_OK);
            oOutMsgBody.mutable_rsp_result()->set_msg("success");
            fin.close();
            SendTo(pChannel, oInMsgHead.cmd(), oInMsgHead.seq(), oOutMsgBody);
            return(true);
        }
        else
        {
            oOutMsgBody.mutable_rsp_result()->set_code(ERR_FILE_NOT_EXIST);
            oOutMsgBody.mutable_rsp_result()->set_msg("file \"" + ssConfFile.str() + "\" not exist!");
            SendTo(pChannel, oInMsgHead.cmd(), oInMsgHead.seq(), oOutMsgBody);
            return(false);
        }
    }
    else
    {
        oOutMsgBody.mutable_rsp_result()->set_code(ERR_PARASE_PROTOBUF);
        oOutMsgBody.mutable_rsp_result()->set_msg("ConfigInfo.ParseFromString() failed.");
        SendTo(pChannel, oInMsgHead.cmd(), oInMsgHead.seq(), oOutMsgBody);
        return(false);
    }
}

} /* namespace neb */
