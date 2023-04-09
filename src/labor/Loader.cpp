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

Loader::Loader(const std::string& strWorkPath, int iWorkerIndex)
    : Worker(strWorkPath, iWorkerIndex, Labor::LABOR_LOADER)
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
        return(GetActorBuilder()->Init());
    }
    return(false);
}

} /* namespace neb */
