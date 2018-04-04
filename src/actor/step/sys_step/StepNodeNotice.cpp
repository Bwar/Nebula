/*******************************************************************************
 * Project:  Nebula
 * @file     StepNodeNotice.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepNodeNotice.hpp"

namespace neb
{

StepNodeNotice::StepNodeNotice(const MsgBody& oMsgBody)
    : m_oMsgBody(oMsgBody)
{
}

StepNodeNotice::~StepNodeNotice()
{
}

E_CMD_STATUS StepNodeNotice::Emit(int iErrno, const std::string& strErrMsg, void* data)
{
    CJsonObject oJson;
    if (!oJson.Parse(m_oMsgBody.data()))
    {
        LOG4_ERROR("failed to parse msgbody content!");
        return(CMD_STATUS_FAULT);
    }
    std::string strNodeType;
    std::string strNodeHost;
    int iNodePort = 0;
    int iWorkerNum = 0;
    char szIdentify[64] = {0};
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
                GetWorkerImpl(this)->AddNodeIdentify(strNodeType, std::string(szIdentify));
                LOG4_DEBUG("AddNodeIdentify(%s,%s)", strNodeType.c_str(), szIdentify);
            }
        }
    }

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
                GetWorkerImpl(this)->DelNodeIdentify(strNodeType, std::string(szIdentify));
                LOG4_DEBUG("DelNodeIdentify(%s,%s)", strNodeType.c_str(), szIdentify);
            }
        }
    }

    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepNodeNotice::Callback(const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepNodeNotice::Timeout()
{
    LOG4_ERROR("timeout!");
    return (CMD_STATUS_FAULT);
}

} /* namespace neb */
