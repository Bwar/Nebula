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

CmdConnectWorker::CmdConnectWorker()
    : pStepConnectWorker(NULL)
{
}

CmdConnectWorker::~CmdConnectWorker()
{
}

bool CmdConnectWorker::AnyMessage(
                const tagChannelContext& stCtx,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    return(true);
}

bool CmdConnectWorker::Start(const tagChannelContext& stCtx, int iWorkerIndex)
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
        pStepConnectWorker = new StepConnectWorker(stCtx, oMsgHead, oMsgBody);
        if (pStepConnectWorker == NULL)
        {
            LOG4_ERROR("error %d: new StepConnectWorker() error!", ERR_NEW);
            return(false);
        }

        if (Register(pStepConnectWorker))
        {
            pStepConnectWorker->Emit(ERR_OK);
            return(true);
        }
        else
        {
            delete pStepConnectWorker;
        }
    }
    return(false);
}

} /* namespace neb */
