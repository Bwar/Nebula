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

namespace neb
{

template <typename ...Targs>
void WorkerImpl::Logger(const std::string& strTraceId, int iLogLevel, Targs... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, std::forward<Targs>(args)...);
}

template <typename ...Targs>
Step* WorkerImpl::NewStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    Step* pStep = dynamic_cast<Step*>(ActorFactory<Targs...>::Instance()->Create(strStepName, std::forward<Targs>(args)...));
    if (nullptr == pStep)
    {
        return(nullptr);
    }
    StepModel* pStepAlias = (StepModel*)pStep;
    pStepAlias->m_dTimeout = (0 == pStepAlias->m_dTimeout) ? m_stWorkerInfo.dStepTimeout : pStepAlias->m_dTimeout;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStepAlias, pStepAlias->m_dTimeout);

    pStepAlias->SetWorker(m_pWorker);
    pStepAlias->SetActiveTime(ev_now(m_loop));
    if (nullptr != pCreator)
    {
        switch(pCreator->m_eActorType)
        {
            case Actor::ACT_PB_STEP:
            case Actor::ACT_HTTP_STEP:
            case Actor::ACT_REDIS_STEP:
            case Actor::ACT_CMD:
            case Actor::ACT_MODULE:
                pStepAlias->m_strTraceId = pCreator->m_strTraceId;
                break;
            case Actor::ACT_SESSION:
            case Actor::ACT_TIMER:
            {
                std::ostringstream oss;
                oss << m_stWorkerInfo.uiNodeId << "." << GetNowTime() << "." << GetSequence();
                pStepAlias->m_strTraceId = std::move(oss.str());
            }
                break;
            default:
                ;
        }
    }
    ev_timer* timer_watcher = pStepAlias->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pStep;
        pStepAlias = pStep = nullptr;
        return(nullptr);
    }

    for (auto iter = pStepAlias->m_setNextStepSeq.begin(); iter != pStepAlias->m_setNextStepSeq.end(); ++iter)
    {
        auto callback_iter = m_mapCallbackStep.find(*iter);
        if (callback_iter != m_mapCallbackStep.end())
        {
            ((StepModel*)callback_iter->second)->m_setPreStepSeq.insert(pStepAlias->GetSequence());
        }
    }

    auto ret = m_mapCallbackStep.insert(std::make_pair(pStepAlias->GetSequence(), pStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pStepAlias->m_dTimeout)
        {
            ev_timer_init (timer_watcher, StepTimeoutCallback, pStepAlias->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStepAlias->GetSequence(), pStepAlias->GetActiveTime(), pStepAlias->GetTimeout());
        return(pStep);
    }
    else
    {
        delete pStep;
        pStepAlias = pStep = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Session* WorkerImpl::NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    Session* pSession = dynamic_cast<Session*>(ActorFactory<Targs...>::Instance()->Create(strSessionName, std::forward<Targs>(args)...));
    if (nullptr == pSession)
    {
        return(nullptr);
    }
    SessionModel* pSessionAlias = (SessionModel*)pSession;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pSession, pSessionAlias->m_dTimeout);

    pSessionAlias->SetWorker(m_pWorker);
    pSessionAlias->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pSessionAlias->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pSession;
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }

    std::pair<std::unordered_map<std::string, Session*>::iterator, bool> ret;
    auto session_name_iter = m_mapCallbackSession.find(pSessionAlias->GetSessionClass());
    if (session_name_iter == m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::make_pair(pSessionAlias->GetSessionId(), pSession));
        m_mapCallbackSession.insert(std::make_pair(pSessionAlias->GetSessionClass(), mapSession));
    }
    else
    {
        ret = session_name_iter->second.insert(std::make_pair(pSessionAlias->GetSessionId(), pSession));
    }
    if (ret.second)
    {
        ev_timer_init (timer_watcher, StepTimeoutCallback, pSessionAlias->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        m_pLogger->WriteLog(Logger::TRACE, "Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSessionAlias->GetSequence(), pSessionAlias->GetActiveTime(), pSessionAlias->GetTimeout());
        return(pSession);
    }
    else
    {
        delete pSession;
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Cmd* WorkerImpl::NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    Cmd* pCmd = dynamic_cast<Cmd*>(ActorFactory<Targs...>::Instance()->Create(strCmdName, std::forward<Targs>(args)...));
    if (nullptr == pCmd)
    {
        return(nullptr);
    }
    CmdModel* pCmdAlias = (CmdModel*)pCmd;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Cmd* 0x%X)", __FUNCTION__, pCmd);

    pCmdAlias->SetWorker(m_pWorker);
    pCmdAlias->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapCmd.insert(std::make_pair(pCmdAlias->GetCmd(), pCmd));
    if (ret.second)
    {
        return(pCmd);
    }
    else
    {
        delete pCmd;
        pCmdAlias = pCmd = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
Module* WorkerImpl::NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    Module* pModule = dynamic_cast<Module*>(ActorFactory<Targs...>::Instance()->Create(strModuleName, std::forward<Targs>(args)...));
    if (nullptr == pModule)
    {
        return(nullptr);
    }
    ModuleModel* pModuleAlias = (ModuleModel*)pModule;
    m_pLogger->WriteLog(Logger::TRACE, "%s(Module* 0x%X)", __FUNCTION__, pModule);

    pModuleAlias->SetWorker(m_pWorker);
    pModuleAlias->SetActiveTime(ev_now(m_loop));

    auto ret = m_mapModule.insert(std::make_pair(pModuleAlias->GetModulePath(), pModule));
    if (ret.second)
    {
        return(pModule);
    }
    else
    {
        delete pModule;
        pModuleAlias = pModule = nullptr;
        return(nullptr);
    }
}

}

#endif /* SRC_LABOR_CHIEF_WORKERIMPL_INL_ */
