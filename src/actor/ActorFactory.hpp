/*******************************************************************************
 * Project:  Nebula
 * @file     ActorFactory.hpp
 * @brief 
 * @author   bwar
 * @date:    Mar 11, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_ACTORFACTORY_HPP_
#define SRC_ACTOR_ACTORFACTORY_HPP_

#include <functional>
#include <string>
#include <unordered_map>
#include "logger/NetLogger.hpp"

namespace neb
{

class Actor;

template<typename ...Targs>
class ActorFactory
{
public:
    static ActorFactory* Instance()
    {
        if (nullptr == s_pActorFactory)
        {
            s_pActorFactory = new ActorFactory();
        }
        return(s_pActorFactory);
    }

    virtual ~ActorFactory(){};

    bool Regist(const std::string& strTypeName, std::function<Actor*(std::shared_ptr<NetLogger>, Targs&&... args)> pFunc);
    bool UnRegist(const std::string& strTypeName);
    Actor* Create(std::shared_ptr<NetLogger> pLogger, const std::string& strTypeName, Targs&&... args);

private:
    ActorFactory(){};
    static ActorFactory<Targs...>* s_pActorFactory;
    std::unordered_map<std::string, std::function<Actor*(std::shared_ptr<NetLogger>, Targs&&...)> > m_mapCreateFunction;
};


template<typename ...Targs>
ActorFactory<Targs...>* ActorFactory<Targs...>::s_pActorFactory = nullptr;

template<typename ...Targs>
bool ActorFactory<Targs...>::Regist(const std::string& strTypeName, std::function<Actor*(std::shared_ptr<NetLogger>, Targs&&... args)> pFunc)
{
    if (nullptr == pFunc)
    {
        return (false);
    }
    bool bReg = m_mapCreateFunction.insert(
                    std::make_pair(strTypeName, pFunc)).second;
    return (bReg);
}

template<typename ...Targs>
bool ActorFactory<Targs...>::UnRegist(const std::string& strTypeName)
{
    auto iter = m_mapCreateFunction.find(strTypeName);
    if (iter == m_mapCreateFunction.end())
    {
        return (false);
    }
    else
    {
        m_mapCreateFunction.erase(iter);
        return (true);
    }
}

template<typename ...Targs>
Actor* ActorFactory<Targs...>::Create(std::shared_ptr<NetLogger> pLogger,
        const std::string& strTypeName, Targs&&... args)
{
    auto iter = m_mapCreateFunction.find(strTypeName);
    if (iter == m_mapCreateFunction.end())
    {
        pLogger->WriteLog(Logger::WARNING, __FILE__, __LINE__, __FUNCTION__,
                "no CreateObject found for \"%s\"", strTypeName.c_str());
        return (nullptr);
    }
    else
    {
        return (iter->second(pLogger, std::forward<Targs>(args)...));
    }
}


} /* namespace neb */

#endif /* SRC_ACTOR_ACTORFACTORY_HPP_ */
