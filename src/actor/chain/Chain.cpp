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

Chain::Chain(const std::string& strChainId, ev_tstamp dChainTimeout)
    : Actor(Actor::ACT_CHAIN, dChainTimeout),
      m_strChainId(strChainId)
{
}

Chain::~Chain()
{
}

void Chain::Init(const std::queue<std::vector<std::string> >& queChainBlock)
{
    m_queChainBlock = queChainBlock;
}

E_CMD_STATUS Chain::NextBlock()
{
    if (m_queChainBlock.empty())
    {
        return(CMD_STATUS_COMPLETED);
    }
    bool bStepInBlock = false;     ///< 当前链块存在Step
    E_CMD_STATUS eResult = CMD_STATUS_START;
    std::vector<std::string>& vecTurnBlocks = m_queChainBlock.front();
    for (auto iter = vecTurnBlocks.begin(); iter != vecTurnBlocks.end(); ++iter)
    {
        LOG4_TRACE("(%s)", (*iter).c_str());
        std::shared_ptr<Matrix> pSharedMatrix = GetMatrix(*iter);
        if (pSharedMatrix == nullptr)
        {
            std::shared_ptr<Actor> pSharedActor = MakeSharedActor(*iter);
            // pSharedMatrix->SetContext(GetContext()); it had been set in MakeSharedActor().
            if (Actor::ACT_MATRIX == pSharedActor->GetActorType())
            {
                pSharedMatrix = std::dynamic_pointer_cast<Matrix>(pSharedActor);
                eResult = pSharedMatrix->Submit();
                pSharedMatrix->SetContext(nullptr);
                if (CMD_STATUS_FAULT == eResult)
                {
                    return(CMD_STATUS_FAULT);
                }
            }
            else if (Actor::ACT_PB_STEP == pSharedActor->GetActorType()
                    || Actor::ACT_HTTP_STEP == pSharedActor->GetActorType()
                    || Actor::ACT_REDIS_STEP == pSharedActor->GetActorType())
            {
                bStepInBlock = true;
                std::shared_ptr<Step> pSharedStep = std::dynamic_pointer_cast<Step>(pSharedActor);
                pSharedStep->SetChainId(GetSequence());
                eResult = pSharedStep->Emit();
                if (CMD_STATUS_FAULT == eResult)
                {
                    return(CMD_STATUS_FAULT);
                }
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
            pSharedMatrix->SetContext(GetContext());
            eResult = pSharedMatrix->Submit();
            pSharedMatrix->SetContext(nullptr);
            if (CMD_STATUS_FAULT == eResult)
            {
                return(CMD_STATUS_FAULT);
            }
        }
    }

    m_queChainBlock.pop();
    if (bStepInBlock)
    {
        return(CMD_STATUS_RUNNING);
    }
    else   // 只有Matrix的链块（无IO回调），执行完当前链块后立即执行下一个链块
    {
        return(NextBlock());
    }
}

} /* namespace neb */
