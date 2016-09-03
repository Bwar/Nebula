/*******************************************************************************
 * Project:  Nebula
 * @file     StepIoTimeout2.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepIoTimeout.hpp"

namespace neb
{

StepIoTimeout::StepIoTimeout(const tagChannelContext& stCtx)
    : m_stCtx(stCtx)
{
}

StepIoTimeout::~StepIoTimeout()
{
}

E_CMD_STATUS StepIoTimeout::Emit(int iErrno, const std::string& strErrMsg,
        void* data)
{
    tagChannelContext* pCtx = tagChannelContext*(data);
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;
    oOutMsgHead.set_cmd(CMD_REQ_BEAT);
    oOutMsgHead.set_seq(GetSequence());
    oOutMsgHead.set_len(0);
    if (SendTo(*pCtx, oOutMsgHead, oOutMsgBody))
    {
        return(CMD_STATUS_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepIoTimeout::Callback(const tagChannelContext& stCtx,
        const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    ((Worker*)m_pLabor)->AddIoTimeout(stCtx);
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepIoTimeout::Timeout()
{
    m_pLabor->Disconnect(m_stCtx);
    return(CMD_STATUS_FAULT);
}


} /* namespace neb */
