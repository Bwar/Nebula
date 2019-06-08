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

    bool Regist(const std::string& strTypeName, std::function<Actor*(Targs&&... args)> pFunc);
    Actor* Create(const std::string& strTypeName, Targs&&... args);

private:
    ActorFactory(){};
    static ActorFactory<Targs...>* s_pActorFactory;
    std::unordered_map<std::string, std::function<Actor*(Targs&&...)> > m_mapCreateFunction;
};


template<typename ...Targs>
ActorFactory<Targs...>* ActorFactory<Targs...>::s_pActorFactory = nullptr;

template<typename ...Targs>
bool ActorFactory<Targs...>::Regist(const std::string& strTypeName, std::function<Actor*(Targs&&... args)> pFunc)
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
Actor* ActorFactory<Targs...>::Create(const std::string& strTypeName, Targs&&... args)
{
    auto iter = m_mapCreateFunction.find(strTypeName);
    if (iter == m_mapCreateFunction.end())
    {
        return (nullptr);
    }
    else
    {
        return (iter->second(std::forward<Targs>(args)...));
    }
}


} /* namespace neb */

#endif /* SRC_ACTOR_ACTORFACTORY_HPP_ */
