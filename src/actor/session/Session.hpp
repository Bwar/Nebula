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

#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class WorkerImpl;

class Session: public Actor
{
public:
    Session(uint32 ulSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "neb::Session");
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "neb::Session");
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    virtual ~Session();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    const std::string& GetSessionId() const
    {
        return(m_strSessionId);
    }

    const std::string& GetSessionClass() const
    {
        return(m_strSessionClassName);
    }

private:
    std::string m_strSessionId;
    std::string m_strSessionClassName;

    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SESSION_HPP_ */
