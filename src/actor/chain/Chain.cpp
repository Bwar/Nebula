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
#include "util/json/CJsonObject.hpp"
#include "actor/context/Context.hpp"
#include "actor/step/Step.hpp"
#include "actor/operator/Operator.hpp"

namespace neb
{

Chain::Chain(const std::string& strChainFlag, ev_tstamp dChainTimeout)
    : Actor(Actor::ACT_CHAIN, dChainTimeout),
      m_uiWaitingStep(0), m_strChainFlag(strChainFlag)
{
}

Chain::~Chain()
{
}

bool Chain::Init(const std::queue<std::vector<std::string> >& queChainBlock)
{
    m_queChainBlock = queChainBlock;
    return(true);
}

bool Chain::Init(CJsonObject& oChainBlock)
{
    LOG4_TRACE("actor chain:  %s", oChainBlock.ToString().c_str());
    if (oChainBlock.IsArray())
    {
        for (int i = 0; i < oChainBlock.GetArraySize(); ++i)
        {
            std::vector<std::string> vecBlock;
            if (oChainBlock[i].IsArray())
            {
                for (int j = 0; j < oChainBlock[i].GetArraySize(); ++j)
                {
                    vecBlock.push_back(oChainBlock[i](j));
                }
            }
            else
            {
                vecBlock.push_back(oChainBlock(i));
            }
            m_queChainBlock.push(std::move(vecBlock));
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

E_CMD_STATUS Chain::Next()
{
    if (m_uiWaitingStep > 0)
    {
        --m_uiWaitingStep;
        if (m_uiWaitingStep > 0)
        {
            return(CMD_STATUS_RUNNING);
        }
    }
    if (m_queChainBlock.empty())
    {
        GetContext()->Done();
        SetContext(nullptr);
        return(CMD_STATUS_COMPLETED);
    }
    E_CMD_STATUS eResult = CMD_STATUS_START;
    std::vector<std::string>& vecTurnBlocks = m_queChainBlock.front();
    for (auto iter = vecTurnBlocks.begin(); iter != vecTurnBlocks.end(); ++iter)
    {
        LOG4_TRACE("(%s)", (*iter).c_str());
        std::shared_ptr<Operator> pSharedOperator = GetOperator(*iter);
        if (pSharedOperator == nullptr)
        {
            std::shared_ptr<Actor> pSharedActor = MakeSharedActor(*iter);
            if (pSharedActor == nullptr)
            {
                LOG4_ERROR("failed to new \"%s\", the chain \"%s\" terminated!",
                        iter->c_str(), m_strChainFlag.c_str());
                break;
            }
            // pSharedOperator->SetContext(GetContext()); it had been set in MakeSharedActor().
            if (Actor::ACT_OPERATOR == pSharedActor->GetActorType())
            {
                pSharedOperator = std::dynamic_pointer_cast<Operator>(pSharedActor);
                eResult = pSharedOperator->Submit();
                pSharedOperator->SetContext(nullptr);
                pSharedOperator->SetTraceId("");
                if (CMD_STATUS_FAULT == eResult)
                {
                    return(CMD_STATUS_FAULT);
                }
            }
            else if (Actor::ACT_PB_STEP == pSharedActor->GetActorType()
                    || Actor::ACT_HTTP_STEP == pSharedActor->GetActorType()
                    || Actor::ACT_REDIS_STEP == pSharedActor->GetActorType())
            {
                std::shared_ptr<Step> pSharedStep = std::dynamic_pointer_cast<Step>(pSharedActor);
                pSharedStep->SetChainId(GetSequence());
                eResult = pSharedStep->Emit();
                if (CMD_STATUS_RUNNING == eResult)
                {
                    ++m_uiWaitingStep;
                }
                else
                {
                    GetLabor(this)->GetActorBuilder()->RemoveStep(pSharedStep);
                    if (CMD_STATUS_FAULT == eResult)
                    {
                        return(CMD_STATUS_FAULT);
                    }
                }
            }
            else
            {
                LOG4_ERROR("\"%s\" is not a Operator or Step, only Operator and Step can be a Chain block.",
                        pSharedActor->GetActorName().c_str());
                return(CMD_STATUS_FAULT);
            }
        }
        else
        {
            pSharedOperator->SetContext(GetContext());
            pSharedOperator->SetTraceId(GetTraceId());
            eResult = pSharedOperator->Submit();
            pSharedOperator->SetContext(nullptr);
            pSharedOperator->SetTraceId("");
            if (CMD_STATUS_FAULT == eResult)
            {
                return(CMD_STATUS_FAULT);
            }
        }
    }

    m_queChainBlock.pop();
    if (m_uiWaitingStep > 0)
    {
        return(CMD_STATUS_RUNNING);
    }
    else   // 只有Operator的链块（无IO回调），执行完当前链块后立即执行下一个链块
    {
        return(Next());
    }
}

E_CMD_STATUS Chain::Timeout()
{
    if (m_uiWaitingStep == 0 && m_queChainBlock.empty())
    {
        return(CMD_STATUS_COMPLETED);
    }
    LOG4_ERROR("chain_id %d timeout, chain flag \"%s\"", GetSequence(), m_strChainFlag.c_str());
    return(CMD_STATUS_FAULT);
}

} /* namespace neb */
