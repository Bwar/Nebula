/*******************************************************************************
 * Project:  Nebula
 * @file     Labor.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月8日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LABOR_LABOR_HPP_
#define LABOR_LABOR_HPP_

#include <ctime>
#include <string>
#include "ev.h"
#include "pb/msg.pb.h"
#include "Definition.hpp"

namespace neb
{

class NodeInfo;
class Actor;
class SocketChannel;
class Dispatcher;
class ActorBuilder;
class CJsonObject;

class Labor
{
public:
    enum LABOR_TYPE
    {
        LABOR_UNDEFINE      = 0,
        LABOR_MANAGER       = 1,
        LABOR_WORKER        = 2,
        LABOR_LOADER        = 3,
    };
public:
    Labor(LABOR_TYPE eLaborType)
        : m_eLaborType(eLaborType)
    {
    }
    Labor(const Labor&) = delete;
    Labor& operator=(const Labor&) = delete;
    virtual ~Labor(){};

    LABOR_TYPE GetLaborType() const
    {
        return(m_eLaborType);
    }

public:
    virtual Dispatcher* GetDispatcher() = 0;
    virtual ActorBuilder* GetActorBuilder() = 0;
    virtual uint32 GetSequence() const = 0;
    virtual time_t GetNowTime() const = 0;
    virtual const CJsonObject& GetNodeConf() const = 0;
    virtual void SetNodeConf(const CJsonObject& oNodeConf) = 0;
    virtual const NodeInfo& GetNodeInfo() const = 0;
    virtual void SetNodeId(uint32 uiNodeId) = 0;
    virtual bool AddNetLogMsg(const MsgBody& oMsgBody) = 0;
    virtual void OnTerminated(struct ev_signal* watcher) = 0;

private:
    LABOR_TYPE m_eLaborType;
};

} /* namespace neb */

#endif /* LABOR_LABOR_HPP_ */
