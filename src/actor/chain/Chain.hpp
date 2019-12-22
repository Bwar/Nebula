/*******************************************************************************
 * Project:  Nebula
 * @file     Chain.hpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月7日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CHAIN_CHAIN_HPP_
#define SRC_ACTOR_CHAIN_CHAIN_HPP_

#include <queue>
#include <vector>
#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class ActorBuilder;
class CJsonObject;

class Chain final: public Actor,
    public neb::DynamicCreator<Chain, std::string&, ev_tstamp>
{
public:
    Chain(const std::string& strChainFlag, ev_tstamp dChainTimeout = 60.0);
    Chain(const Chain&) = delete;
    Chain& operator=(const Chain&) = delete;
    virtual ~Chain();

    bool Init(const std::queue<std::vector<std::string> >& queChainBlock);
    bool Init(CJsonObject& oChainBlock);
    E_CMD_STATUS Next();

    virtual E_CMD_STATUS Timeout();

    const std::string& GetChainFlag() const
    {
        return(m_strChainFlag);
    }

private:
    uint32 m_uiWaitingStep;
    std::string m_strChainFlag;
    // queue的每个元素称为链块（std::unordered_set<std::string>）
    std::queue<std::vector<std::string> > m_queChainBlock;

    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CHAIN_CHAIN_HPP_ */
