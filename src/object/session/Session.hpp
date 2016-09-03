/*******************************************************************************
 * Project:  Nebula
 * @file     Session.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月12日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_SESSION_SESSION_HPP_
#define SRC_OBJECT_SESSION_SESSION_HPP_

#include "object/Object.hpp"

namespace neb
{

class Worker;

class Session: public Object
{
public:
    Session(uint32 ulSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "oss::Session");
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0, const std::string& strSessionClass = "oss::Session");
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

    friend class Worker;
};

} /* namespace neb */

#endif /* SRC_OBJECT_SESSION_SESSION_HPP_ */
