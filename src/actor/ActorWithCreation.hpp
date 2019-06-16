/*******************************************************************************
 * Project:  Nebula
 * @file     ActorWithCreation.hpp
 * @brief    带创建功能的Actor
 * @author   Bwar
 * @date:    2019年6月15日
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTORWITHCREATION_ACTORWITHCREATION_HPP_
#define SRC_ACTORWITHCREATION_ACTORWITHCREATION_HPP_

#include "Actor.hpp"

namespace neb
{

class ActorWithCreation: public Actor
{
public:
    ActorWithCreation(Actor::ACTOR_TYPE eActorWithCreationType = Actor::ACT_UNDEFINE, ev_tstamp dTimeout = 0.0);
    ActorWithCreation(const ActorWithCreation&) = delete;
    ActorWithCreation& operator=(const ActorWithCreation&) = delete;
    virtual ~ActorWithCreation();

    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
};

template <typename ...Targs>
void ActorWithCreation::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> ActorWithCreation::MakeSharedStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> ActorWithCreation::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTORWITHCREATION_ACTORWITHCREATION_HPP_ */
