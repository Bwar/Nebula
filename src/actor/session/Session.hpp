/*******************************************************************************
 * Project:  Nebula
 * @file     Session.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_SESSION_SESSION_HPP_
#define SRC_ACTOR_SESSION_SESSION_HPP_

#include <queue>
#include "actor/DynamicCreator.hpp"
#include "actor/Actor.hpp"

namespace neb
{

class ActorBuilder;

class Session: public Actor
{
public:
    Session(uint32 uiSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    virtual ~Session();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

    const std::string& GetSessionId() const
    {
        return(m_strSessionId);
    }

protected:
    // 这两个构造函数专为Timer而用，其他Session子类不可使用
    Session(ACTOR_TYPE eActorType, uint32 ulSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(ACTOR_TYPE eActorType, const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);

private:
    uint32 PopWaitingStep();

private:
    std::string m_strSessionId;
    friend class ActorBuilder;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SESSION_HPP_ */
