/*******************************************************************************
 * Project:  Nebula
 * @file     RedisStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/step/RedisStep.hpp"
#include <algorithm>

namespace neb
{

RedisStep::RedisStep(std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : Step(ACT_REDIS_STEP, pNextStep, dTimeout)
{
}

RedisStep::~RedisStep()
{
}

void RedisStep::SetHashKey(const std::string& strHashKey)
{
    m_strHashKey = strHashKey;
}

void RedisStep::SetCmd(const std::string& strCmd)
{
    m_vecCmdArguments.clear();
    m_strCmd = strCmd;
    std::transform(m_strCmd.begin(), m_strCmd.end(), m_strCmd.begin(), [](unsigned char c)->unsigned char{return std::toupper(c);});
}

void RedisStep::Append(const std::string& strArgument, bool bIsBinaryArg)
{
    m_vecCmdArguments.push_back(std::make_pair(strArgument, bIsBinaryArg));
}

std::string RedisStep::CmdToString() const
{
    std::string strCmd = m_strCmd;
    std::vector<std::pair<std::string, bool> >::const_iterator c_iter = m_vecCmdArguments.begin();
    for (; c_iter != m_vecCmdArguments.end(); c_iter++)
    {
        if (c_iter->second)
        {
            strCmd = strCmd + " a_binary_arg";
        }
        else
        {
            strCmd = strCmd + " " + c_iter->first;
        }
    }
    return strCmd;
}

const RedisRequest& RedisStep::GenrateRedisRequest()
{
    m_oRedisRequest.Clear();
    m_oRedisRequest.set_type(REDIS_REPLY_ARRAY);
    auto pElement = m_oRedisRequest.add_element();
    pElement->set_type(REDIS_REPLY_STRING);
    pElement->set_str(m_strCmd);
    for (size_t i = 0; i < m_vecCmdArguments.size(); ++i)
    {
        pElement = m_oRedisRequest.add_element();
        pElement->set_type(REDIS_REPLY_STRING);
        pElement->set_str(m_vecCmdArguments[i].first);
    }
    return(m_oRedisRequest);
}

std::shared_ptr<RedisRequest>  RedisStep::MutableRedisRequest()
{
    auto pRequest = std::make_shared<RedisRequest>();
    pRequest->set_type(REDIS_REPLY_ARRAY);
    auto pElement = pRequest->add_element();
    pElement->set_type(REDIS_REPLY_STRING);
    pElement->set_str(m_strCmd);
    for (size_t i = 0; i < m_vecCmdArguments.size(); ++i)
    {
        pElement = pRequest->add_element();
        pElement->set_type(REDIS_REPLY_STRING);
        pElement->set_str(m_vecCmdArguments[i].first);
    }
    return(pRequest);
}

} /* namespace neb */
