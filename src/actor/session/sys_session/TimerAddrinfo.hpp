/*******************************************************************************
 * Project:  Nebula
 * @file     TimerAddrinfo.hpp
 * @brief    
 * @author   Bwar
 * @date:    2022-08-27
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_SESSION_SYS_SESSION_TIMERADDRINFO_HPP_
#define SRC_ACTOR_SESSION_SYS_SESSION_TIMERADDRINFO_HPP_

#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "actor/session/Timer.hpp"
#include "actor/ActorSys.hpp"

namespace neb
{

class TimerAddrinfo: public neb::Timer,
    public DynamicCreator<TimerAddrinfo, const std::string&>,
    public ActorSys
{
public:
    TimerAddrinfo(const std::string& strSessionId);
    virtual ~TimerAddrinfo();

    virtual E_CMD_STATUS Timeout();

    struct addrinfo* GetAddrinfo()
    {
        return(m_pAddrInfo);
    }

    void MigrateAddrinfo(struct addrinfo* pAddrInfo);

private:
    struct addrinfo* m_pAddrInfo;
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SYS_SESSION_TIMERADDRINFO_HPP_ */

