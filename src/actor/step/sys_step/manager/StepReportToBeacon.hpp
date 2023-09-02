/*******************************************************************************
 * Project:  Nebula
 * @file     StepReportToBeacon.hpp
 * @brief    向注册中心上报
 * @author   Bwar
 * @date:    2019年9月22日
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef SRC_ACTOR_STEP_SYS_STEP_MANAGER_STEPREPORTTOBEACON_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_MANAGER_STEPREPORTTOBEACON_HPP_

#include "actor/ActorSys.hpp"
#include "actor/step/PbStep.hpp"

namespace neb
{

class SessionManager;

class StepReportToBeacon: public PbStep,
        public DynamicCreator<StepReportToBeacon, ev_tstamp>, public ActorSys
{
public:
    StepReportToBeacon(ev_tstamp dTimeout);
    virtual ~StepReportToBeacon();

    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);

    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual E_CMD_STATUS Timeout();

private:
    std::shared_ptr<SessionManager> m_pSessionManager;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_MANAGER_STEPREPORTTOBEACON_HPP_ */
