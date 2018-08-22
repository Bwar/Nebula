/*******************************************************************************
 * Project:  Nebula
 * @file     SessionLogger.hpp
 * @brief    Worker写网络日志会话
 * @author   bwar
 * @date:    Auguest 19, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SESSIONLOGGER_HPP_
#define SESSIONLOGGER_HPP_

#include <list>
#include "pb/msg.pb.h"
#include <actor/session/Timer.hpp>

namespace neb
{

class SessionLogger: public Timer, DynamicCreator<SessionLogger>
{
public:
    SessionLogger();
    virtual ~SessionLogger();

    virtual E_CMD_STATUS Timeout();

    void AddMsg(const MsgBody& oMsgBody);
                    
private:
    std::list<MsgBody> m_listLogMsgBody;
};

}

#endif /* SESSIONLOGGER_HPP_ */
