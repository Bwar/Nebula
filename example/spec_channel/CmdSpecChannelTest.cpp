#include "CmdSpecChannelTest.hpp"
#include <util/encrypt/crc16.h>
#include <labor/NodeInfo.hpp>
#include <channel/SocketChannel.hpp>
#include <actor/step/RedisStep.hpp>
#include "CmdOfdb.hpp"

namespace neb
{

CmdSpecChannelTest::CmdSpecChannelTest(int32 iCmd)
    : neb::RedisCmd(iCmd)
{
}

CmdSpecChannelTest::~CmdSpecChannelTest()
{
}

bool CmdSpecChannelTest::Init()
{
    return(true);
}

bool CmdSpecChannelTest::AnyMessage(
        std::shared_ptr<neb::SocketChannel> pChannel,
        const neb::RedisMsg& oRedisMsg)
{
    LOG4_DEBUG("%s", oRedisMsg.DebugString().c_str());
    std::string strCmd = oRedisMsg.element(0).str();
    std::transform(strCmd.begin(), strCmd.end(), strCmd.begin(), [](unsigned char c)->unsigned char{return std::toupper(c);});
    if (ofdb::CmdOfdb::IsValidCmd(strCmd))
    {
        if (strCmd == "PING")
        {
            Pong(pChannel, oRedisMsg);
            return(true);
        }
        if (strCmd == "COMMAND")
        {
            Command(pChannel, oRedisMsg);
            return(true);
        }
        if (strCmd == "QUIT")
        {
            Quit(pChannel, oRedisMsg);
            return(true);
        }
    }
    else
    {
        LOG4_ERROR("ERR unknown command %s!", oRedisMsg.element(0).str());
        SendErrorReply(pChannel, "ERR unknown command '" + oRedisMsg.element(0).str() + "'");
        return(false);
    }

    uint16 uiSlotNumPerWorker = 16384 / GetNodeInfo().uiWorkerNum;
    uint16 uiSlotFrom = uiSlotNumPerWorker * (GetWorkerIndex() - 1);
    uint16 uiSlotTo = (uiSlotNumPerWorker * GetWorkerIndex()) - 1;
    uint16 uiSlotId = HashSlot(oRedisMsg.element(1).str().data(), oRedisMsg.element(1).str().length());
    LOG4_DEBUG("TTTTTTTTTTTTTTTTT:   worker %u slot from %u to %u, and slot_id = %u",
                        GetWorkerIndex(), uiSlotFrom, uiSlotTo, uiSlotId);
    if (uiSlotId >= uiSlotFrom && uiSlotId <= uiSlotTo)
    {
        char szResult[256] = {0};
        neb::RedisReply oRedisReply;
        snprintf(szResult, 256, "slot %u, reply from worker %u", uiSlotId, GetWorkerIndex());
        LOG4_DEBUG("TTTTTTTTTTTTTTTTT:   %s", szResult);
        oRedisReply.set_type(REDIS_REPLY_STRING);
        oRedisReply.set_str(szResult);
        if (pChannel->GetCodecType() == CODEC_TRANSFER) // spec channel
        {
            uint32 uiPeerStepSeq = pChannel->GetPeerStepSeq();
            if (uiPeerStepSeq == 0)
            {
                LOG4_ERROR("spec channel uiPeerStepSeq is 0");
                return(false);
            }
            SetPeerStepSeq(uiPeerStepSeq);
        }
        return(SendTo(pChannel, oRedisReply));
    }
    else
    {
        uint32 uiToWorker = uiSlotId / uiSlotNumPerWorker;
        if (uiSlotId % uiSlotNumPerWorker != 0)
        {
            uiToWorker++;
        }
        LOG4_DEBUG("TTTTTTTTTTTTTTTTT:   transmit to %u", uiToWorker);
        auto pStep = MakeSharedStep("neb::StepSpecChannelTest", pChannel, oRedisMsg, uiToWorker);
        if (pStep != nullptr)
        {
            pStep->Emit();
        }
    }
    return(true);
}

void CmdSpecChannelTest::Command(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg)
{
    if (m_oCommand.element_size() == 0)
    {
        m_oCommand.set_type(neb::REDIS_REPLY_ARRAY);
        auto pElement = m_oCommand.add_element();
        pElement->set_type(neb::REDIS_REPLY_ARRAY);
        auto pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_STRING);
        pE->set_str("HMGET");
        pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_INTEGER);
        pE->set_integer(-3);
        pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_ARRAY);
        auto e = pE->add_element();
        e->set_type(neb::REDIS_REPLY_STATUS);
        e->set_str("readonly");
        e = pE->add_element();
        e->set_type(neb::REDIS_REPLY_STATUS);
        e->set_str("fast");
        pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_INTEGER);
        pE->set_integer(1);
        pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_INTEGER);
        pE->set_integer(1);
        pE = pElement->add_element();
        pE->set_type(neb::REDIS_REPLY_INTEGER);
        pE->set_integer(1);
    }
    SendTo(pChannel, m_oCommand);
}

void CmdSpecChannelTest::Pong(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg)
{
    if (oRedisMsg.element_size() > 1 && oRedisMsg.element(1).str().size() > 0)
    {
        neb::RedisReply oPong;
        oPong.set_type(neb::REDIS_REPLY_STATUS);
        oPong.set_str(oRedisMsg.element(1).str());
        SendTo(pChannel, oPong);
    }
    else
    {
        if (m_oPong.element_size() == 0)
        {
            m_oPong.set_type(neb::REDIS_REPLY_STATUS);
            m_oPong.set_str("PONG");
        }
        SendTo(pChannel, m_oPong);
    }
}

void CmdSpecChannelTest::Quit(std::shared_ptr<neb::SocketChannel> pChannel, const neb::RedisMsg& oRedisMsg)
{
    neb::RedisReply oReply;
    oReply.set_type(neb::REDIS_REPLY_STATUS);
    oReply.set_str("OK");
    SendTo(pChannel, oReply);
}

void CmdSpecChannelTest::SendErrorReply(
        std::shared_ptr<neb::SocketChannel> pChannel, const std::string& strErrMsg)
{
    neb::RedisReply oRedisReply;
    oRedisReply.set_type(neb::REDIS_REPLY_ERROR);
    oRedisReply.set_str(strErrMsg);
    SendTo(pChannel, oRedisReply);
}


uint16 CmdSpecChannelTest::HashSlot(const char* key, uint32 keylen)
{
    uint32 s, e; /* start-end indexes of { and } */

    /* Search the first occurrence of '{'. */
    for (s = 0; s < keylen; s++)
        if (key[s] == '{') break;

    /* No '{' ? Hash the whole key. This is the base case. */
    if (s == keylen) return crc16(key,keylen) & 16383;

    /* '{' found? Check if we have the corresponding '}'. */
    for (e = s+1; e < keylen; e++)
        if (key[e] == '}') break;

    /* No '}' or nothing between {} ? Hash the whole key. */
    if (e == keylen || e == s+1) return crc16(key,keylen) & 16383;

    /* If we are here there is both a { and a } on its right. Hash
     * what is in the middle between { and }. */
    return crc16(key+s+1,e-s-1) & 16383;
}

} /* namespace neb */

