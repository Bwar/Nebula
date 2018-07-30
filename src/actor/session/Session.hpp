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
    Session(uint32 ulSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    virtual ~Session();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Cmd> MakeSharedCmd(const std::string& strCmdName, Targs... args);
    template <typename ...Targs> std::shared_ptr<Module> MakeSharedModule(const std::string& strModuleName, Targs... args);

private:
    friend class WorkerImpl;
};

template <typename ...Targs>
void Session::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs... args)
{
    m_pWorker->Logger(m_strTraceId, iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template <typename ...Targs>
std::shared_ptr<Step> Session::MakeSharedStep(const std::string& strStepName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strStepName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Session> Session::MakeSharedSession(const std::string& strSessionName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strSessionName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Cmd> Session::MakeSharedCmd(const std::string& strCmdName, Targs... args)
{
    return(m_pWorker->MakeSharedStep(this, strCmdName, std::forward<Targs>(args)...));
}

template <typename ...Targs>
std::shared_ptr<Module> Session::MakeSharedModule(const std::string& strModuleName, Targs... args)
{
    return(m_pWorker->MakeSharedSession(this, strModuleName, std::forward<Targs>(args)...));
}

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SESSION_HPP_ */
