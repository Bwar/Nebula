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
    for (int i = 0; i < oJson["node_arry_reg"].GetArraySize(); ++i)
    {
        if (oJson["node_arry_reg"][i].Get("node_type",strNodeType)
                && oJson["node_arry_reg"][i].Get("node_ip",strNodeHost)
                && oJson["node_arry_reg"][i].Get("node_port",iNodePort)
                && oJson["node_arry_reg"][i].Get("worker_num",iWorkerNum))
        {
            for(int j = 0; j < iWorkerNum; ++j)
            {
                snprintf(szIdentify, sizeof(szIdentify), "%s:%d.%d", strNodeHost.c_str(), iNodePort, j);
                AddNodeIdentify(strNodeType, std::string(szIdentify));
                LOG4_DEBUG("AddNodeIdentify(%s,%s)", strNodeType.c_str(), szIdentify);
            }
        }
    }

    for (int i = 0; i < oJson["node_arry_exit"].GetArraySize(); ++i)
    {
        if (oJson["node_arry_exit"][i].Get("node_type",strNodeType)
                && oJson["node_arry_exit"][i].Get("node_ip",strNodeHost)
                && oJson["node_arry_exit"][i].Get("node_port",iNodePort)
                && oJson["node_arry_exit"][i].Get("worker_num",iWorkerNum))
        {
            for(int j = 0; j < iWorkerNum; ++j)
            {
                snprintf(szIdentify, sizeof(szIdentify), "%s:%d.%d", strNodeHost.c_str(), iNodePort, j);
                DelNodeIdentify(strNodeType, std::string(szIdentify));
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
