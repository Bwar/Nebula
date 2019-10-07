/*******************************************************************************
 * Project:  Nebula
 * @file     CmdNodeNotice.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdNodeNotice.hpp"
#include "util/CBuffer.hpp"
#include "util/json/CJsonObject.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

CmdNodeNotice::CmdNodeNotice(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdNodeNotice::~CmdNodeNotice()
{
}

bool CmdNodeNotice::AnyMessage(
                std::shared_ptr<SocketChannel> pChannel,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    CBuffer oBuff;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;

    CJsonObject oJson;
    if (oJson.Parse(oInMsgBody.data()))
    {
        LOG4_DEBUG("CmdNodeNotice seq[%llu] jsonbuf[%s] Parse is ok",
            oInMsgHead.seq(),oInMsgBody.data().c_str());        
        std::string strNodeType;
        std::string strNodeHost;
        int iNodePort = 0;
        int iWorkerNum = 0;
        char szIdentify[64] = {0};
        for (int i = 0; i < oJson["del_nodes"].GetArraySize(); ++i)
        {
            if (oJson["del_nodes"][i].Get("node_type",strNodeType)
                    && oJson["del_nodes"][i].Get("node_ip",strNodeHost)
                    && oJson["del_nodes"][i].Get("node_port",iNodePort)
                    && oJson["del_nodes"][i].Get("worker_num",iWorkerNum))
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    snprintf(szIdentify, sizeof(szIdentify), "%s:%d.%d", strNodeHost.c_str(), iNodePort, j);
                    GetLabor(this)->GetDispatcher()->DelNodeIdentify(strNodeType, std::string(szIdentify));
                    LOG4_DEBUG("DelNodeIdentify(%s,%s)", strNodeType.c_str(), szIdentify);
                }
            }
        }

        for (int i = 0; i < oJson["add_nodes"].GetArraySize(); ++i)
        {
            if (oJson["add_nodes"][i].Get("node_type",strNodeType)
                    && oJson["add_nodes"][i].Get("node_ip",strNodeHost)
                    && oJson["add_nodes"][i].Get("node_port",iNodePort)
                    && oJson["add_nodes"][i].Get("worker_num",iWorkerNum))
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    snprintf(szIdentify, sizeof(szIdentify), "%s:%d.%d", strNodeHost.c_str(), iNodePort, j);
                    GetLabor(this)->GetDispatcher()->AddNodeIdentify(strNodeType, std::string(szIdentify));
                    LOG4_DEBUG("AddNodeIdentify(%s,%s)", strNodeType.c_str(), szIdentify);
                }
            }
        }
    }
    else
    {
        LOG4_ERROR("failed to parse %s", oInMsgBody.data().c_str());
    }

    return(false);
}

} /* namespace neb */
