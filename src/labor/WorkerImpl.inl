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
void WorkerImpl::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
void WorkerImpl::Logger(const std::string& strTraceId, int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Actor> WorkerImpl::MakeSharedActor(Actor* pCreator, const std::string& strActorName, Targs... args)
{
    Actor* pActor = ActorFactory<Targs...>::Instance()->Create(strActorName, std::forward<Targs>(args)...);
    if (nullptr == pActor)
    {
        LOG4_ERROR("failed to make shared actor \"%s\"", strActorName.c_str());
        return(nullptr);
    }
    pActor->SetWorker(m_pWorker);
    pActor->SetActiveTime(ev_now(m_loop));
    pActor->SetActorName(strActorName);
    if (nullptr != pCreator)
    {
        pActor->SetContext(pCreator->GetContext());
    }
    std::shared_ptr<Actor> pSharedActor;
    pSharedActor.reset(pActor);
    pActor = nullptr;
    switch (pSharedActor->GetActorType())
    {
        case Actor::ACT_PB_STEP:
        case Actor::ACT_HTTP_STEP:
        case Actor::ACT_REDIS_STEP:
            if (TransformToSharedStep(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_SESSION:
        case Actor::ACT_TIMER:
        case Actor::ACT_CONTEXT:
            if (TransformToSharedSession(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_CMD:
            if (TransformToSharedCmd(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_MODULE:
            if (TransformToSharedModule(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_MATRIX:
            if (TransformToSharedMatrix(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        case Actor::ACT_CHAIN:
            if (TransformToSharedChain(pCreator, pSharedActor))
            {
                return(pSharedActor);
            }
            break;
        default:
            LOG4_ERROR("\"%s\" must be a Step, a Session, a Matrix, a Cmd or a Module.",
                    strActorName.c_str());
            return(nullptr);
    }
    return(nullptr);
}

template <typename ...Targs>
std::shared_ptr<Step> WorkerImpl::MakeSharedStep(Actor* pCreator, const std::string& strStepName, Targs... args)
{
    return(std::dynamic_pointer_cast<Step>(MakeSharedActor(pCreator, strStepName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Session> WorkerImpl::MakeSharedSession(Actor* pCreator, const std::string& strSessionName, Targs... args)
{
    return(std::dynamic_pointer_cast<Session>(MakeSharedActor(pCreator, strSessionName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Cmd> WorkerImpl::MakeSharedCmd(Actor* pCreator, const std::string& strCmdName, Targs... args)
{
    return(std::dynamic_pointer_cast<Cmd>(MakeSharedActor(pCreator, strCmdName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Module> WorkerImpl::MakeSharedModule(Actor* pCreator, const std::string& strModuleName, Targs... args)
{
    return(std::dynamic_pointer_cast<Module>(MakeSharedActor(pCreator, strModuleName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Matrix> WorkerImpl::MakeSharedMatrix(Actor* pCreator, const std::string& strMatrixName, Targs... args)
{
    return(std::dynamic_pointer_cast<Matrix>(MakeSharedActor(pCreator, strMatrixName, std::forward<Targs>(args)...)));
}

template <typename ...Targs>
std::shared_ptr<Chain> WorkerImpl::MakeSharedChain(Actor* pCreator, const std::string& strChainName, Targs... args)
{
    return(std::dynamic_pointer_cast<Chain>(MakeSharedActor(pCreator, strChainName, std::forward<Targs>(args)...)));
}

}

#endif /* LABOR_WORKERIMPL_INL_ */
