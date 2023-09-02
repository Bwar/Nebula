/*******************************************************************************
 * Project:  Nebula
 * @file     GrpcStep.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-14
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_GRPCSTEP_HPP_
#define SRC_ACTOR_STEP_GRPCSTEP_HPP_

#include "HttpStep.hpp"

namespace neb
{

class GrpcStep: public HttpStep
{
public:
    GrpcStep(ev_tstamp dTimeout = gc_dConfigTimeout);
    GrpcStep(const GrpcStep&) = delete;
    GrpcStep& operator=(const GrpcStep&) = delete;
    virtual ~GrpcStep();

    virtual E_CMD_STATUS Callback(
                    std::shared_ptr<SocketChannel> pChannel,
                    const HttpMsg& oHttpMsg,
                    void* data = NULL);

    virtual E_CMD_STATUS Callback(
                    std::shared_ptr<SocketChannel> pChannel,
                    const std::string& strGrpcResponse,
                    int iStatus, const std::string& strStatusMessage) = 0;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_GRPCSTEP_HPP_ */
