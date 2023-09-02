/*******************************************************************************
 * Project:  Nebula
 * @file     StepNebualChannelPing.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/step/sys_step/StepNebualChannelPing.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

StepNebualChannelPing::StepNebualChannelPing(std::shared_ptr<SocketChannel> pChannel)
    : m_pChannel(pChannel)
{
}

StepNebualChannelPing::~StepNebualChannelPing()
{
}

E_CMD_STATUS StepNebualChannelPing::Emit(int iErrno, const std::string& strErrMsg,
        void* data)
{
    MsgBody oOutMsgBody;
    oOutMsgBody.set_trace_id(GetTraceId());
    if (IO<CodecNebula>::SendRequest(this, m_pChannel, CMD_REQ_BEAT, GetSequence(), oOutMsgBody))
    {
        return(CMD_STATUS_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(CMD_STATUS_FAULT);
    }
}

E_CMD_STATUS StepNebualChannelPing::Callback(std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead, const MsgBody& oInMsgBody, void* data)
{
    GetLabor(this)->GetDispatcher()->AddIoTimeout(pChannel);
    return(CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepNebualChannelPing::Timeout()
{
    GetLabor(this)->GetDispatcher()->Disconnect(m_pChannel);
    return(CMD_STATUS_FAULT);
}


} /* namespace neb */
