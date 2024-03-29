/*******************************************************************************
 * Project:  Nebula
 * @file     Step.cpp
 * @brief 
 * @author   bwar
 * @date:    Apr 4, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include "Step.hpp"

namespace neb
{

Step::Step(Actor::ACTOR_TYPE eActorType, ev_tstamp dTimeout)
    : Actor(eActorType, dTimeout),
      m_uiChainId(0)
{
}

Step::~Step()
{
}

E_CMD_STATUS Step::Callback(std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oMsgHead, const MsgBody& oMsgBody, void* data)
{
    if (Actor::ACT_PB_STEP != GetActorType())
    {
        LOG4_WARNING("got a PbStep callback, but this step is not a PbStep, "
                "you need to implement a PbStep callback");
    }
    return(CMD_STATUS_FAULT);
}

E_CMD_STATUS Step::Callback(std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oHttpMsg, void* data)
{
    if (Actor::ACT_HTTP_STEP != GetActorType())
    {
        LOG4_WARNING("got a HttpStep callback, but this step is not a HttpStep, "
                "you need to implement a HttpStep callback");
    }
    return(CMD_STATUS_FAULT);
}

E_CMD_STATUS Step::Callback(std::shared_ptr<SocketChannel> pChannel,
            const RedisReply& oRedisReply)
{
    if (Actor::ACT_REDIS_STEP != GetActorType())
    {
        LOG4_WARNING("got a RedisStep callback, but this step is not a RedisStep, "
                "you need to implement a RedisStep callback");
    }
    return(CMD_STATUS_FAULT);
}

E_CMD_STATUS Step::Callback(std::shared_ptr<SocketChannel> pChannel,
            const char* pRawData, uint32 uiRawDataSize)
{
    if (Actor::ACT_RAW_STEP != GetActorType())
    {
        LOG4_WARNING("got a RawStep callback, but this step is not a RawStep, "
                "you need to implement a RawStep callback");
    }
    return(CMD_STATUS_FAULT);
}

void Step::SetChainId(uint32 uiChainId)
{
    m_uiChainId = uiChainId;
}

} /* namespace neb */
