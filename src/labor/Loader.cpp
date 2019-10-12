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

namespace neb
{

Loader::Loader(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex)
    : Worker(strWorkPath, iControlFd, iDataFd, iWorkerIndex, Labor::LABOR_LOADER)
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
        return(GetActorBuilder()->Init(
                (const_cast<CJsonObject&>(GetNodeConf()))["load_config"]["loader"]["boot_load"],
                (const_cast<CJsonObject&>(GetNodeConf()))["load_config"]["loader"]["dynamic_loading"]));
    }
    return(false);
}

} /* namespace neb */
