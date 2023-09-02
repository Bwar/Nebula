/*******************************************************************************
 * Project:  Nebula
 * @file     GrpcModule.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-13
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_CMD_GRPCMODULE_HPP_
#define SRC_ACTOR_CMD_GRPCMODULE_HPP_

#include "Module.hpp"
#include "codec/CodecUtil.hpp"
#include "codec/grpc/Grpc.hpp"

namespace neb
{

class GrpcModule: public Module
{
public:
    GrpcModule(const std::string& strModulePath);
    virtual ~GrpcModule();

    virtual bool Init()
    {
        return(true);
    }

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            const HttpMsg& oHttpMsg
            ) override final;

    virtual bool AnyMessage(
            std::shared_ptr<SocketChannel> pChannel,
            uint32 uiStreamId,
            const std::string& strGrpcRequest) = 0;
};

}

#endif
