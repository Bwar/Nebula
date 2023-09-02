/*******************************************************************************
 * Project:  Nebula
 * @file     CassStep.hpp
 * @brief 
 * @author   Bwar
 * @date:    2021-12-11
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_CASSSTEP_HPP_
#define SRC_ACTOR_STEP_CASSSTEP_HPP_

#include "Step.hpp"
#include "codec/cass/CassMessage.hpp"
#include "codec/cass/CassRequest.hpp"
#include "codec/cass/CassResponse.hpp"
#include "codec/cass/result/CassResult.hpp"

namespace neb
{

class CassStep: public Step
{
public:
    CassStep(ev_tstamp dTimeout = gc_dConfigTimeout);
    CassStep(const CassStep&) = delete;
    CassStep& operator=(const CassStep&) = delete;
    virtual ~CassStep();

    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const CassMessage& oCassMsg);

    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const CassResult& oCassResult,
            int iErrCode, const std::string& strErrMsg) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_CASSSTEP_HPP_ */
