/*******************************************************************************
 * Project:  Nebula
 * @file     Timer.hpp
 * @brief 
 * @author   Bwar
 * @date:    2018年8月5日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_SESSION_TIMER_HPP_
#define SRC_ACTOR_SESSION_TIMER_HPP_

#include "labor/Worker.hpp"
#include "actor/DynamicCreator.hpp"
#include "Session.hpp"

namespace neb
{

class Timer: public Session
{
public:
    Timer(uint32 ulSessionId, ev_tstamp dSessionTimeout = 60.0);
    Timer(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    virtual ~Timer();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    virtual void SetActiveTime(ev_tstamp dActiveTime)
    {
        ;
    }

private:
    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_TIMER_HPP_ */
