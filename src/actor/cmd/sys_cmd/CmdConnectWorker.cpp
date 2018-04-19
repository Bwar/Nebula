/*******************************************************************************
 * Project:  Nebula
 * @file     CmdConnectWorker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/cmd/sys_cmd/CmdConnectWorker.hpp>

namespace neb
{

CmdConnectWorker::CmdConnectWorker(int iCmd)
    : Cmd(iCmd), pStepConnectWorker(nullptr)
{
}

CmdConnectWorker::~CmdConnectWorker()
{
}

bool CmdConnectWorker::AnyMessage(
                std::shared_ptr<SocketChannel> pChannel,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    return(true);
}

bool CmdConnectWorker::Start(std::shared_ptr<SocketChannel> pChannel, int iWorkerIndex)
{
    MsgHead oMsgHead;
    MsgBody oMsgBody;
    ConnectWorker oConnWorker;
    oConnWorker.set_worker_index(iWorkerIndex);
    oMsgBody.set_data(oConnWorker.SerializeAsString());
    oMsgHead.set_cmd(CMD_REQ_CONNECT_TO_WORKER);
//    oMsgHead.set_seq(pStepConnectWorker->GetSequence());
    oMsgHead.set_len(oMsgBody.ByteSize());
    LOG4_DEBUG("send cmd %d.", oMsgHead.cmd());
    for (int i = 0; i < 3; ++i)
    {
        pStepConnectWorker = std::dynamic_pointer_cast<StepConnectWorker>(MakeSharedStep("neb::StepConnectWorker", pChannel, oMsgHead, oMsgBody));
        if (nullptr == pStepConnectWorker)
        {
            LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
            return(false);
        }
        pStepConnectWorker->Emit(ERR_OK);
        return(true);
    }
    return(false);
}

} /* namespace neb */
