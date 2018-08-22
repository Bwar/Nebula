/*******************************************************************************
 * Project:  Nebula
 * @file     SessionManagerLogger.hpp
 * @brief    Manager写网络日志会话
 * @author   bwar
 * @date:    Auguest 19, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SESSIONMANAGERLOGGER_HPP_
#define SESSIONMANAGERLOGGER_HPP_

#include <list>
#include "pb/msg.pb.h"
#include "labor/Manager.hpp"

namespace neb
{

class Manager;

class SessionManagerLogger
{
public:
    SessionManagerLogger(Manager* pManager);
    virtual ~SessionManagerLogger();

    virtual E_CMD_STATUS Timeout();

    void AddMsg(const MsgBody& oMsgBody);
                    
private:
    Manager* m_pManager;
    std::list<MsgBody> m_listLogMsgBody;
};

}

#endif /* SESSIONMANAGERLOGGER_HPP_ */
