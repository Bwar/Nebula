/*******************************************************************************
 * Project:  Nebula
 * @file     SessionWorkerThreadInfo.cpp
 * @brief    数据上报
 * @author   Bwar
 * @date:    2020-10-03
 * @note
 * Modify history:
 ******************************************************************************/
#include "SessionWorkerThreadInfo.hpp"

namespace neb
{

SessionWorkerThreadInfo::SessionWorkerThreadInfo(const std::string& strSessionId, const std::vector<uint64>& vecWorkerThreadId)
    : neb::Session(strSessionId), m_vecWorkerThreadId(vecWorkerThreadId)
{
}

SessionWorkerThreadInfo::~SessionWorkerThreadInfo()
{
}

}

