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

#include "labor/Worker.hpp"
#include "actor/DynamicCreator.hpp"
#include "SessionModel.hpp"

namespace neb
{

class Session: public SessionModel
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
    template <typename ...Targs> void Logger(int iLogLevel, Targs... args);
    template <typename ...Targs> Step* NewStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> Session* NewSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> Cmd* NewCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> Module* NewModule(const std::string& strModuleName, Targs... args);

private:
    friend class WorkerImpl;
};

template <typename ...Targs>
void Session::Logger(int iLogLevel, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, std::forward<Targs>(args)...);
}

template <typename ...Targs>
Step* Session::NewStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->NewStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Session* Session::NewSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->NewSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Cmd* Session::NewCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->NewStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
Module* Session::NewModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->NewSession(this, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SESSION_HPP_ */
