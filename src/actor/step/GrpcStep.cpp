/*******************************************************************************
 * Project:  Nebula
 * @file     GrpcStep.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-14
 * @note
 * Modify history:
 ******************************************************************************/
#include "GrpcStep.hpp"
#include "util/StringConverter.hpp"

namespace neb
{

GrpcStep::GrpcStep(std::shared_ptr<Step> pNextStep, ev_tstamp dTimeout)
    : HttpStep(pNextStep, dTimeout)
{
}

GrpcStep::~GrpcStep()
{
}

E_CMD_STATUS GrpcStep::Callback(
        std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, void* data)
{
    uint8 ucCompressedFlag = 0;
    uint32 uiMessageLength = 0;
    std::string strResponseData;
    ucCompressedFlag = oHttpMsg.body()[0];
    uiMessageLength |= (oHttpMsg.body()[4] & 0xFF);
    uiMessageLength |= ((oHttpMsg.body()[3] & 0xFF) << 8);
    uiMessageLength |= ((oHttpMsg.body()[2] & 0xFF) << 16);
    uiMessageLength |= ((oHttpMsg.body()[1] & 0xFF) << 24);
    if (ucCompressedFlag)
    {
        auto iter = oHttpMsg.headers().find("grpc-encoding");
        if (iter == oHttpMsg.headers().end())
        {
            LOG4_ERROR("Compressed-Flag had been set, but no \"grpc-encoding\" found in header.");
            return(Callback(pChannel, strResponseData, GRPC_INVALID_ARGUMENT,
                    "Compressed-Flag had been set, but no \"grpc-encoding\" found in header."));
        }
        else
        {
            if (iter->second == "gzip")
            {
                if (!CodecUtil::Gunzip(oHttpMsg.body().substr(5, uiMessageLength), strResponseData))
                {
                    LOG4_ERROR("failed to gunzip message.");
                    return(Callback(pChannel, strResponseData, GRPC_INVALID_ARGUMENT, "failed to gunzip message."));
                }
            }
            else
            {
                LOG4_ERROR("compression algorithm not support.");
                return(Callback(pChannel, strResponseData, GRPC_INVALID_ARGUMENT, "compression algorithm not support."));
            }
        }
    }
    else
    {
        strResponseData.assign(oHttpMsg.body(), 5, uiMessageLength);
    }

    int iStatus = 0;
    std::string strStatusMsg;
    for (int i = 0; i < oHttpMsg.trailer_header_size(); ++i)
    {
        if ("grpc-status" == oHttpMsg.trailer_header(i).name())
        {
            iStatus = StringConverter::RapidAtoi<int32>(oHttpMsg.trailer_header(i).value().c_str());
        }
        else if ("grpc-message" == oHttpMsg.trailer_header(i).name())
        {
            strStatusMsg = oHttpMsg.trailer_header(i).value();
        }
    }
    return(Callback(pChannel, strResponseData, iStatus, strStatusMsg));
}

} /* namespace neb */

