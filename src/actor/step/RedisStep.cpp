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

namespace neb
{

RedisStep::RedisStep(std::shared_ptr<Step> pNextStep)
    : Step(ACT_REDIS_STEP, pNextStep, gc_dNoTimeout)
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

} /* namespace neb */
