/*******************************************************************************
 * Project:  Nebula
 * @file     StepRedisChannelPing.cpp
 * @brief 
 * @author   Bwar
 * @date:    2023-01-24
 * @note
 * Modify history:
 ******************************************************************************/
#include "StepRedisChannelPing.hpp"
#include "ios/IO.hpp"

namespace neb
{

StepRedisChannelPing::StepRedisChannelPing(std::shared_ptr<SocketChannel> pChannel)
    : m_pChannel(pChannel)
{
}

StepRedisChannelPing::~StepRedisChannelPing()
{
}

E_CMD_STATUS StepRedisChannelPing::Emit(int iErrno, const std::string& strErrMsg,
        void* data)
{
    SetCmd("PING");
    LOG4_TRACE("send 'PING' to channel[%d] %s", m_pChannel->GetFd(), m_pChannel->GetIdentify().c_str());
    if (IO<CodecResp>::SendRequest(this, m_pChannel, GenerateRedisRequest()))
    {
        return(CMD_STATUS_RUNNING);
    }
    else        // SendTo错误会触发断开连接和回收资源
    {
        return(CMD_STATUS_FAULT);
    }
}

neb::E_CMD_STATUS StepRedisChannelPing::Callback(
    std::shared_ptr<neb::SocketChannel> pChannel,
    const neb::RedisReply& oRedisReply)
{
    GetLabor(this)->GetDispatcher()->AddIoTimeout(pChannel, pChannel->GetKeepAlive());
    return(neb::CMD_STATUS_COMPLETED);
}

E_CMD_STATUS StepRedisChannelPing::Timeout()
{
    GetLabor(this)->GetDispatcher()->Disconnect(m_pChannel);
    return(CMD_STATUS_FAULT);
}

} /* namespace neb */

