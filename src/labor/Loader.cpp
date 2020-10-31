/*******************************************************************************
 * Project:  Nebula
 * @file     Loader.hpp
 * @brief    装载者，持有者
 * @author   Bwar
 * @date:    2019年10月4日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Loader.hpp"
#include "actor/ActorBuilder.hpp"
#include "actor/Actor.hpp"

namespace neb
{

Loader::Loader(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex)
    : Worker(strWorkPath, iControlFd, iDataFd, iWorkerIndex, Labor::LABOR_LOADER)
{
}

Loader::Loader(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, const std::vector<uint64>& vecWorkerThreadId)
    : Worker(strWorkPath, iControlFd, iDataFd, iWorkerIndex, Labor::LABOR_LOADER),
      m_vecWorkerThreadId(vecWorkerThreadId)
{
}

Loader::~Loader()
{
}

bool Loader::InitActorBuilder()
{
    LOG4_TRACE("");
    if (NewActorBuilder())
    {
        std::string strSessionId = "neb::SessionWorkerThreadInfo";
        GetActorBuilder()->MakeSharedActor(nullptr, "neb::SessionWorkerThreadInfo", strSessionId, m_vecWorkerThreadId);
        return(GetActorBuilder()->Init(
                (const_cast<CJsonObject&>(GetNodeConf()))["load_config"]["loader"]["boot_load"],
                (const_cast<CJsonObject&>(GetNodeConf()))["load_config"]["loader"]["dynamic_loading"]));
    }
    return(false);
}

} /* namespace neb */
