/*******************************************************************************
 * Project:  Nebula
 * @file     GrpcModule.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-13
 * @note
 * Modify history:
 ******************************************************************************/
#include "GrpcModule.hpp"
#include "util/StringConverter.hpp"

namespace neb
{

GrpcModule::GrpcModule(const std::string& strModulePath)
    : Module(strModulePath)
{
}

GrpcModule::~GrpcModule()
{
}

bool GrpcModule::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    char szLength[5] = {0};
    uint8 ucCompressedFlag = 0;
    uint32 uiMessageLength = 0;
    std::string strMessage;
    ucCompressedFlag = oHttpMsg.body()[0];
    szLength[0] = oHttpMsg.body()[4];
    szLength[1] = oHttpMsg.body()[3];
    szLength[2] = oHttpMsg.body()[2];
    szLength[3] = oHttpMsg.body()[1];
    uiMessageLength = StringConverter::RapidAtoi<uint32>(szLength);
    if (ucCompressedFlag)
    {
        auto iter = oHttpMsg.headers().find("grpc-encoding");
        if (iter == oHttpMsg.headers().end())
        {
            LOG4_ERROR("Compressed-Flag had been set, but no \"grpc-encoding\" found in header.");
            ActorSender::SendTo(this, pChannel, oHttpMsg.stream_id(),
                    "", GRPC_UNAVAILABLE, "Compressed-Flag had been set, but no \"grpc-encoding\" found in header.");
            return(false);
        }
        else
        {
            if (iter->second == "gzip")
            {
                if (!CodecUtil::Gunzip(oHttpMsg.body().substr(5, uiMessageLength), strMessage))
                {
                    LOG4_ERROR("failed to gunzip message.");
                    return(false);
                }
            }
            else
            {
                LOG4_ERROR("compression algorithm not support.");
                return(false);
            }
        }
    }
    else
    {
        strMessage.assign(oHttpMsg.body(), 5, uiMessageLength);
    }
    return(AnyMessage(pChannel, oHttpMsg.stream_id(), strMessage));
}

} /* namespace neb */

