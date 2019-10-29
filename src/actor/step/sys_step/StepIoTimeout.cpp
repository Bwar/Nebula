/*******************************************************************************
 * Project:  Nebula
 * @file     StepIoTimeout2.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/step/sys_step/StepIoTimeout.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

StepIoTimeout::StepIoTimeout(std::shared_ptr<SocketChannel> pChannel)
    : m_pChannel(pChannel)
{
}

StepIoTimeout::~StepIoTimeout()
{
}

E_CMD_STATUS StepIoTimeout::Emit(int iErrno, const std::string& strErrMsg,
        void* data)
{
    MsgBody oOutMsgBody;
    if (SendTo(m_pChannel, CMD_REQ_BEAT, GetSequence(), oOutMsgBody))
    {
        return(CMD_STATUS_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepIoTimeout::Callback(std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    GetLabor(this)->GetDispatcher()->AddIoTimeout(pChannel);
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepIoTimeout::Timeout()
{
    GetLabor(this)->GetDispatcher()->Disconnect(m_pChannel);
    return(CMD_STATUS_FAULT);
}


} /* namespace neb */
