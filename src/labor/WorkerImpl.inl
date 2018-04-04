/*******************************************************************************
* Project:  Nebula
* @file     WorkerImpl.inl
* @brief 
* @author   bwar
* @date:    Feb 26, 2018
* @note
* Modify history:
******************************************************************************/
#ifndef LABOR_CHIEF_WORKERIMPL_INL_
#define LABOR_CHIEF_WORKERIMPL_INL_


#include "actor/ActorFactory.hpp"


template <typename ...Targs>
void WorkerImpl::Logger(const std::string& strTraceId, int iLogLevel, Targs... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, std::forward<Targs>(args)...);
}

template <typename ...Targs>
Step* WorkerImpl::NewStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    Step* pStep = ActorFactory<Targs...>::Instance()->Create(strStepName, std::forward<Targs>(args)...);
    if (nullptr == pStep)
    {
        return(nullptr);
    }
    pStep->m_dTimeout = (0 == pStep->m_dTimeout) ? m_stWorkerInfo.dStepTimeout : pStep->m_dTimeout;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, pStep->m_dTimeout);

    pStep->SetWorker(m_pWorker);
    pStep->SetActiveTime(ev_now(m_loop));
    if (nullptr != pCreator)
    {
        switch(pCreator->m_eActorType)
        {
            case Actor::ACT_PB_STEP:
            case Actor::ACT_HTTP_STEP:
            case Actor::ACT_REDIS_STEP:
            case Actor::ACT_CMD:
            case Actor::ACT_MODULE:
                pStep->m_strTraceId = pCreator->m_strTraceId;
                break;
            case Actor::ACT_SESSION:
            case Actor::ACT_TIMER:
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                pStep->m_strTraceId = std::move(oss.str());
            }
                break;
            default:
                ;
        }
    }
    ev_timer* timer_watcher = pStep->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pStep;
        return(nullptr);
    }

    for (auto iter = pStep->m_setNextStepSeq.begin(); iter != pStep->m_setNextStepSeq.end(); ++iter)
    {
        auto callback_iter = m_mapCallbackStep.find(*iter);
        if (callback_iter != m_mapCallbackStep.end())
        {
            callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
        }
    }

    auto ret = m_mapCallbackStep.insert(std::make_pair(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pStep->m_dTimeout)
        {
            ev_timer_init (timer_watcher, StepTimeoutCallback, pStep->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStep->GetSequence(), pStep->GetActiveTime(), pStep->GetTimeout());
        return(pStep);
    }
    else
    {
        delete pStep;
        return(nullptr);
    }
}

template <typename ...Targs>
Session* WorkerImpl::NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    Session* pSession = ActorFactory<Targs...>::Instance()->Create(strSessionName, std::forward<Targs>(args)...);
    if (nullptr == pSession)
    {
        return(nullptr);
    }
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pSession, pSession->m_dTimeout);

    pSession->SetWorker(m_pWorker);
    pSession->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pSession->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pSession;
        return(nullptr);
    }

    std::pair<std::unordered_map<std::string, Session*>::iterator, bool> ret;
    auto session_name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (session_name_iter == m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::make_pair(pSession->GetSessionId(), pSession));
        m_mapCallbackSession.insert(std::make_pair(pSession->GetSessionClass(), mapSession));
    }
    else
    {
        ret = session_name_iter->second.insert(std::make_pair(pSession->GetSessionId(), pSession));
    }
    if (ret.second)
    {
        ev_timer_init (timer_watcher, StepTimeoutCallback, pSession->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSession->GetSequence(), pSession->GetActiveTime(), pSession->GetTimeout());
        return(pSession);
    }
    else
    {
        delete pSession;
        return(nullptr);
    }
}

template <typename ...Targs>
Cmd* WorkerImpl::NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    Cmd* pCmd = ActorFactory<Targs...>::Instance()->Create(strCmdName, std::forward<Targs>(args)...);
    if (nullptr == pCmd)
    {
        return(nullptr);
    }
    m_pLogger->WriteLog(Logger::TRACE, "%s(Cmd* 0x%X)", __FUNCTION__, pCmd);

    pCmd->SetWorker(m_pWorker);
    pCmd->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapCmd.insert(std::make_pair(pCmd->GetCmd(), pCmd));
    if (ret.second)
    {
        return(pCmd);
    }
    else
    {
        delete pCmd;
        return(nullptr);
    }
}

template <typename ...Targs>
Module* WorkerImpl::NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    Module* pModule = ActorFactory<Targs...>::Instance()->Create(strModuleName, std::forward<Targs>(args)...);
    if (nullptr == pModule)
    {
        return(nullptr);
    }
    m_pLogger->WriteLog(Logger::TRACE, "%s(Module* 0x%X)", __FUNCTION__, pModule);

    pModule->SetWorker(m_pWorker);
    pModule->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapModule.insert(std::make_pair(pModule->GetModulePath(), pModule));
    if (ret.second)
    {
        return(pModule);
    }
    else
    {
        delete pModule;
        return(nullptr);
    }
}

#endif /* SRC_LABOR_CHIEF_WORKERIMPL_INL_ */
