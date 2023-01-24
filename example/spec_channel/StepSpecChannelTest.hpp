#ifndef SRC_TEST_STEPSPECCHANNELTEST_HPP_
#define SRC_TEST_STEPSPECCHANNELTEST_HPP_

#include <actor/step/RedisStep.hpp>

namespace neb
{

class StepSpecChannelTest: public neb::RedisStep,
    public neb::DynamicCreator<StepSpecChannelTest,
        std::shared_ptr<neb::SocketChannel>&, const neb::RedisMsg&, uint32&>
{
public:
    StepSpecChannelTest(
            std::shared_ptr<neb::SocketChannel> pChannel,
            const neb::RedisMsg& oRedisMsg, uint32 uiToWorker);
    virtual ~StepSpecChannelTest();

    virtual neb::E_CMD_STATUS Emit(int iErrno, const std::string& strErrMsg, void* data = NULL);

    virtual neb::E_CMD_STATUS Callback(
            std::shared_ptr<neb::SocketChannel> pChannel,
            const neb::RedisReply& oRedisReply);
    virtual neb::E_CMD_STATUS ErrBack(std::shared_ptr<neb::SocketChannel> pChannel,
                int iErrno, const std::string& strErrMsg);

    virtual neb::E_CMD_STATUS Timeout();

private:
    uint32 m_uiToWorker;
    std::shared_ptr<neb::SocketChannel> m_pReqChannel;
    neb::RedisMsg m_oReqRedisMsg;
};

} /* namespace neb */

#endif /* SRC_TEST_STEPSPECCHANNELTEST_HPP_ */

