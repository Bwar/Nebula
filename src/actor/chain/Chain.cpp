/*******************************************************************************
 * Project:  Nebula
 * @file     Chain.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月7日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Chain.hpp"
#include "actor/step/Step.hpp"
#include "actor/matrix/Matrix.hpp"

namespace neb
{

Chain::Chain(ev_tstamp dChainTimeout)
    : ActorWithCreation(Actor::ACT_CHAIN, dChainTimeout)
{
}

Chain::~Chain()
{
}

void Chain::InitChainBlock(const std::vector<std::string>& vecChainBlock)
{
    m_vecChainBlock = vecChainBlock;
}

E_CMD_STATUS Chain::NextBlock()
{
    auto iter = m_vecChainBlock.begin();
    LOG4_TRACE("(%s)", (*iter).c_str());
    std::shared_ptr<Matrix> pSharedMatrix = GetMatrix(*iter);
    if (pSharedMatrix == nullptr)
    {
        std::shared_ptr<Actor> pSharedActor = m_pWorker->MakeSharedActor(this, *iter);
        // pSharedMatrix->SetContext(GetContext()); it had been set in MakeSharedActor().
        m_vecChainBlock.erase(iter);
        if (Actor::ACT_MATRIX == pSharedActor->GetActorType())
        {
            if (CMD_STATUS_FAULT
                    == (std::dynamic_pointer_cast<Matrix>(pSharedActor))->Launch())
            {
                return(CMD_STATUS_FAULT);
            }
            return(NextBlock());
        }
        else if (Actor::ACT_PB_STEP == pSharedActor->GetActorType()
                || Actor::ACT_HTTP_STEP == pSharedActor->GetActorType()
                || Actor::ACT_REDIS_STEP == pSharedActor->GetActorType())
        {
            std::shared_ptr<Step> pSharedStep = std::dynamic_pointer_cast<Step>(pSharedActor);
            pSharedStep->SetChainId(GetSequence());
            return(pSharedStep->Emit());
        }
        else
        {
            LOG4_ERROR("\"%s\" is not a Matrix or Step, only Matrix and Step can be a Chain block.",
                    pSharedActor->GetActorName().c_str());
            return(CMD_STATUS_FAULT);
        }
    }
    else
    {
        m_vecChainBlock.erase(iter);
        pSharedMatrix->SetContext(GetContext());
        if (CMD_STATUS_FAULT == pSharedMatrix->Launch())
        {
            return(CMD_STATUS_FAULT);
        }
        return(NextBlock());
    }
}

} /* namespace neb */
