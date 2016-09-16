/*******************************************************************************
 * Project:  neb
 * @file     RedisCmd.cpp
 * @brief 
 * @author   lbh
 * @date:    2015年8月15日
 * @note
 * Modify history:
 ******************************************************************************/
#include "RedisCmd.hpp"

namespace neb
{

RedisCmd::RedisCmd()
{
}

RedisCmd::~RedisCmd()
{
}

void RedisCmd::SetHashKey(const std::string& strHashKey)
{
    m_strHashKey = strHashKey;
}

void RedisCmd::SetCmd(const std::string& strCmd)
{
    m_vecCmdArguments.clear();
    m_strCmd = strCmd;
}

void RedisCmd::Append(const std::string& strArgument, bool bIsBinaryArg)
{
    m_vecCmdArguments.push_back(std::make_pair(strArgument, bIsBinaryArg));
}

std::string RedisCmd::ToString() const
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
