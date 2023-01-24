#include "StepSpecChannelTest.hpp"
#include <ios/IO.hpp>

namespace neb
{

StepSpecChannelTest::StepSpecChannelTest(
        std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg, uint32 uiToWorker)
    : neb::RedisStep(2.0),
      m_uiToWorker(uiToWorker), m_pReqChannel(pChannel), m_oReqRedisMsg(oRedisMsg)
{
}

StepSpecChannelTest::~StepSpecChannelTest()
{
}

neb::E_CMD_STATUS StepSpecChannelTest::Emit(int iErrno, const std::string& strErrMsg, void* data)
{
    if (IO<CodecResp>::TransmitTo(this, m_uiToWorker, GetSequence(), m_oReqRedisMsg))
    {
        return(neb::CMD_STATUS_RUNNING);
    }
    LOG4_ERROR("IO<CodecResp>::TransmitTo failed.");
    return(neb::CMD_STATUS_FAULT);
}

neb::E_CMD_STATUS StepSpecChannelTest::Callback(
    std::shared_ptr<neb::SocketChannel> pChannel,
    const neb::RedisReply& oRedisReply)
{
    if (SendTo(m_pReqChannel, oRedisReply))
    {
        LOG4_ERROR("failed to send response.");
    }
    return(neb::CMD_STATUS_COMPLETED);
}

neb::E_CMD_STATUS StepSpecChannelTest::ErrBack(
    std::shared_ptr<neb::SocketChannel> pChannel, int iErrno, const std::string& strErrMsg)
{
    LOG4_ERROR("%s", strErrMsg.c_str());
    return(neb::CMD_STATUS_FAULT);
}

neb::E_CMD_STATUS StepSpecChannelTest::Timeout()
{
    LOG4_ERROR("");
    return(neb::CMD_STATUS_FAULT);
}

} /* namespace neb */

