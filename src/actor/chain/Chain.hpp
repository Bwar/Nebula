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
#include "labor/Worker.hpp"
#include "actor/ActorWithCreation.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Chain final: public ActorWithCreation
{
public:
    Chain(const std::string& strChainId, ev_tstamp dChainTimeout = 60.0);
    Chain(const Chain&) = delete;
    Chain& operator=(const Chain&) = delete;
    virtual ~Chain();

    void Init(const std::queue<std::vector<std::string> >& queChainBlock);
    virtual E_CMD_STATUS Timeout()
    {
        return(CMD_STATUS_FAULT);
    }

    const std::string& GetChainId() const
    {
        return(m_strChainId);
    }

private:
    E_CMD_STATUS NextBlock();

private:
    std::string m_strChainId;
    // queue的每个元素称为链块（std::unordered_set<std::string>）
    std::queue<std::vector<std::string> > m_queChainBlock;

    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CHAIN_CHAIN_HPP_ */
