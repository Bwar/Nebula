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

#include <vector>
#include "labor/Worker.hpp"
#include "actor/ActorWithCreation.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Chain: public ActorWithCreation
{
public:
    Chain(ev_tstamp dChainTimeout = 60.0);
    Chain(const Chain&) = delete;
    Chain& operator=(const Chain&) = delete;
    virtual ~Chain();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

private:
    void InitChainBlock(const std::vector<std::string>& vecChainBlock);
    E_CMD_STATUS NextBlock();

private:
    std::vector<std::string> m_vecChainBlock;

    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_CHAIN_CHAIN_HPP_ */
