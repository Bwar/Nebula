#ifndef SRC_TEST_CMDSPECCHANNELTEST_HPP_
#define SRC_TEST_CMDSPECCHANNELTEST_HPP_

#include <actor/cmd/RedisCmd.hpp>

namespace neb
{

class CmdSpecChannelTest: public neb::RedisCmd,
    public neb::DynamicCreator<CmdSpecChannelTest, int32>
{
public:
    CmdSpecChannelTest(int32 iCmd);
    virtual ~CmdSpecChannelTest();

    virtual bool Init();

    virtual bool AnyMessage(
            std::shared_ptr<neb::SocketChannel> pChannel,
            const neb::RedisMsg& oRedisMsg);

protected:
    void Command(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg);
    void Pong(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg);
    void Quit(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg);
    void SendErrorReply(std::shared_ptr<neb::SocketChannel> pChannel, const std::string& strErrMsg);
    uint16 HashSlot(const char* key, uint32 keylen);

private:
    neb::RedisReply m_oCommand;
    neb::RedisReply m_oPong;
};

} /* namespace neb */

#endif /* SRC_TEST_CMDSPECCHANNELTEST_HPP_ */

