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
void WorkerImpl::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
void WorkerImpl::Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> WorkerImpl::NewStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    Step* pStep = dynamic_cast<Step*>(ActorFactory<Targs...>::Instance()->Create(strStepName, std::forward<Targs>(args)...));
    if (nullptr == pStep)
    {
        return(nullptr);
    }
    StepModel* pStepAlias = (StepModel*)pStep;
    pStepAlias->m_dTimeout = (0 == pStepAlias->m_dTimeout) ? m_stWorkerInfo.dStepTimeout : pStepAlias->m_dTimeout;
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStepAlias, pStepAlias->m_dTimeout);

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
            (std::dynamic_pointer_cast<StepModel>(callback_iter->second))->m_setPreStepSeq.insert(pStepAlias->GetSequence());
        }
    }

    std::shared_ptr<Step> pSharedStep(pStep);
    auto ret = m_mapCallbackStep.insert(std::make_pair(pStepAlias->GetSequence(), pStep));
    if (ret.second)
    {
        if (gc_dNoTimeout != pStepAlias->m_dTimeout)
        {
            ev_timer_init (timer_watcher, StepTimeoutCallback, pStepAlias->m_dTimeout + ev_time() - ev_now(m_loop), 0.);
            ev_timer_start (m_loop, timer_watcher);
        }
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStepAlias->GetSequence(), pStepAlias->GetActiveTime(), pStepAlias->GetTimeout());
        return(pSharedStep);
    }
    else
    {
        pStepAlias = pStep = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
std::shared_ptr<Session> WorkerImpl::NewSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    Session* pSession = dynamic_cast<Session*>(ActorFactory<Targs...>::Instance()->Create(strSessionName, std::forward<Targs>(args)...));
    if (nullptr == pSession)
    {
        return(nullptr);
    }
    SessionModel* pSessionAlias = (SessionModel*)pSession;
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pSession, pSessionAlias->m_dTimeout);

    pSessionAlias->SetWorker(m_pWorker);
    pSessionAlias->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pSessionAlias->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        delete pSession;
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }

    std::shared_ptr<Session> pSharedSession(pSession);
    std::pair<std::unordered_map<std::string, std::shared_ptr<Session>>::iterator, bool> ret;
    auto session_name_iter = m_mapCallbackSession.find(pSessionAlias->GetSessionClass());
    if (session_name_iter == m_mapCallbackSession.end())
    {
        std::unordered_map<std::string, std::shared_ptr<Session>> mapSession;
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
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pSessionAlias->GetSequence(), pSessionAlias->GetActiveTime(), pSessionAlias->GetTimeout());
        return(pSharedSession);
    }
    else
    {
        pSessionAlias = pSession = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
std::shared_ptr<Cmd> WorkerImpl::NewCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    Cmd* pCmd = dynamic_cast<Cmd*>(ActorFactory<Targs...>::Instance()->Create(strCmdName, std::forward<Targs>(args)...));
    if (nullptr == pCmd)
    {
        return(nullptr);
    }
    CmdModel* pCmdAlias = (CmdModel*)pCmd;
    LOG4_TRACE("%s(Cmd* 0x%X)", __FUNCTION__, pCmd);

    pCmdAlias->SetWorker(m_pWorker);
    pCmdAlias->SetActiveTime(ev_now(m_loop));

    std::shared_ptr<Cmd> pSharedCmd(pCmd);
    auto ret = m_mapCmd.insert(std::make_pair(pCmdAlias->GetCmd(), pSharedCmd));
    if (ret.second)
    {
        return(pSharedCmd);
    }
    else
    {
        pCmdAlias = pCmd = nullptr;
        return(nullptr);
    }
}

template <typename ...Targs>
std::shared_ptr<Module> WorkerImpl::NewModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    Module* pModule = dynamic_cast<Module*>(ActorFactory<Targs...>::Instance()->Create(strModuleName, std::forward<Targs>(args)...));
    if (nullptr == pModule)
    {
        return(nullptr);
    }
    ModuleModel* pModuleAlias = (ModuleModel*)pModule;
    LOG4_TRACE("%s(Module* 0x%X)", __FUNCTION__, pModule);

    pModuleAlias->SetWorker(m_pWorker);
    pModuleAlias->SetActiveTime(ev_now(m_loop));

    std::shared_ptr<Module> pSharedModule(pModule);
    auto ret = m_mapModule.insert(std::make_pair(pModuleAlias->GetModulePath(), pSharedModule));
    if (ret.second)
    {
        return(pSharedModule);
    }
    else
    {
        pModuleAlias = pModule = nullptr;
        return(nullptr);
    }
}

}

#endif /* SRC_LABOR_CHIEF_WORKERIMPL_INL_ */
