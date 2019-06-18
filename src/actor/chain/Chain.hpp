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
#include <unordered_set>
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

    void Init(const std::queue<std::unordered_set<std::string> >& queChainBlock);
    /**
     * @brief 调用链超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

    const std::string& GetChainId() const
    {
        return(m_strChainId);
    }

private:
    E_CMD_STATUS NextBlock();

private:
    std::string m_strChainId;
    // queue的每个元素称为链块（std::unordered_set<std::string>）
    std::queue<std::unordered_set<std::string> > m_queChainBlock;

    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CHAIN_CHAIN_HPP_ */
