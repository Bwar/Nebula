/*******************************************************************************
 * Project:  Nebula
 * @file     SessionWorkerThreadInfo.hpp
 * @brief    数据上报
 * @author   Bwar
 * @date:    2019-12-22
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef ACTOR_SESSION_SYS_SESSION_SESSIONWORKERTHREADINFO_HPP_
#define ACTOR_SESSION_SYS_SESSION_SESSIONWORKERTHREADINFO_HPP_

#include <string>
#include <unordered_map>
#include "actor/session/Session.hpp"
#include "actor/ActorSys.hpp"
#include "pb/report.pb.h"

namespace neb
{

class SessionWorkerThreadInfo: public Session,
    DynamicCreator<SessionWorkerThreadInfo, std::string&, std::vector<uint64>&>,
    public ActorSys
{
public:
    SessionWorkerThreadInfo(const std::string& strSessionId, const std::vector<uint64>& vecWorkerThreadId);
    virtual ~SessionWorkerThreadInfo();

    virtual E_CMD_STATUS Timeout()
    {
        return(CMD_STATUS_RUNNING);
    }

    const std::vector<uint64>& GetWorkerThreadId() const
    {
        return(m_vecWorkerThreadId);
    }

private:
    std::vector<uint64> m_vecWorkerThreadId;
};

}

#endif /* ACTOR_SESSION_SYS_SESSION_SESSIONWORKERTHREADINFO_HPP_ */
