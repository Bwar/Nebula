/*******************************************************************************
 * Project:  Nebula
 * @file     ActorSender.cpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-13
 * @note     
 * Modify history:
 ******************************************************************************/
#include "ActorSender.hpp"
#include <algorithm>
#include "Actor.hpp"
#include "ios/Dispatcher.hpp"
#include "codec/CodecHttp.hpp"
//#include "actor/session/Session.hpp"
//#include "actor/step/Step.hpp"
//#include "labor/Worker.hpp"
#include "ios/IO.hpp"

namespace neb
{

ActorSender::ActorSender()
{
}

ActorSender::~ActorSender()
{
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel)
{
    return(IO<void>::Send(pChannel));
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pActor->GetTraceId());
    return(IO<CodecNebula>::SendResponse(pActor, pChannel, iCmd, uiSeq, oMsgBody));
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"x-trace-id", pActor->GetTraceId()});
    return(IO<CodecHttp>::SendResponse(pActor, pChannel, oHttpMsg));
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const RedisReply& oRedisReply)
{
    return(IO<CodecResp>::SendResponse(pActor, pChannel, oRedisReply));
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const char* pRawData, uint32 uiRawDataSize)
{
    return(IO<CodecRaw>::SendResponse(pActor, pChannel, pRawData, uiRawDataSize));
}

bool ActorSender::SendTo(Actor* pActor, const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    ChannelOption stOption;
    stOption.bPipeline = true;
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pActor->GetTraceId());
    return(IO<CodecNebula>::SendTo(pActor, strIdentify, stOption, iCmd, uiSeq, oMsgBody));
}

bool ActorSender::SendTo(Actor* pActor, const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq)
{
    ChannelOption stOption;
    if (oHttpMsg.headers().find("x-trace-id") == oHttpMsg.headers().end())
    {
        (const_cast<HttpMsg&>(oHttpMsg)).mutable_headers()->insert({"x-trace-id", pActor->GetTraceId()});
    }
    if (oHttpMsg.http_major() >= 2)
    {
        stOption.bPipeline = true;
    }
    std::string strSchema = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(":"));
    std::transform(strSchema.begin(), strSchema.end(), strSchema.begin(), [](unsigned char c)->unsigned char {return std::tolower(c);});
    if (strSchema == std::string("https"))
    {
        stOption.bWithSsl = true;
    }
    if (oHttpMsg.http_major() == 2)
    {
        return(IO<CodecHttp2>::SendTo(pActor, strHost, iPort, stOption, oHttpMsg));
    }
    else
    {
        return(IO<CodecHttp>::SendTo(pActor, strHost, iPort, stOption, oHttpMsg));
    }
}

bool ActorSender::SendTo(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    ChannelOption stOption;
    stOption.bPipeline = bPipeline;
    stOption.bPipeline = bWithSsl;
    return(IO<CodecResp>::SendTo(pActor, strIdentify, SOCKET_STREAM, stOption, oRedisMsg));
}

bool ActorSender::SendToCluster(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline, bool bEnableReadOnly)
{
    return(pActor->m_pLabor->GetActorBuilder()->SendToCluster(strIdentify, bWithSsl, bPipeline, oRedisMsg, pActor->GetSequence(), bEnableReadOnly));
}

bool ActorSender::SendRoundRobin(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline)
{
    ChannelOption stOption;
    stOption.bPipeline = bPipeline;
    stOption.bPipeline = bWithSsl;
    return(IO<CodecResp>::SendRoundRobin(pActor, strIdentify, stOption, oRedisMsg));
}

bool ActorSender::SendTo(Actor* pActor, const std::string& strIdentify, const char* pRawData, uint32 uiRawDataSize, bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    ChannelOption stOption;
    stOption.bPipeline = bPipeline;
    stOption.bPipeline = bWithSsl;
    return(IO<CodecRaw>::SendTo(pActor, strIdentify, stOption, pRawData, uiRawDataSize));
}

bool ActorSender::SendRoundRobin(Actor* pActor, const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    ChannelOption stOption;
    stOption.bPipeline = true;
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pActor->GetTraceId());
    return(IO<CodecNebula>::SendRoundRobin(pActor, strNodeType, stOption, iCmd, uiSeq, oMsgBody));
}

bool ActorSender::SendOriented(Actor* pActor, const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    ChannelOption stOption;
    stOption.bPipeline = true;
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pActor->GetTraceId());
    if (eCodecType == CODEC_NEBULA)
    {
        return(IO<CodecNebula>::SendOriented(pActor, strNodeType, uiFactor, stOption, iCmd, uiSeq, oMsgBody));
    }
    else if (eCodecType == CODEC_NEBULA_IN_NODE)
    {
        return(IO<CodecNebulaInNode>::SendOriented(pActor, strNodeType, uiFactor, stOption, iCmd, uiSeq, oMsgBody));
    }
    else
    {
        return(IO<CodecProto>::SendOriented(pActor, strNodeType, uiFactor, stOption, iCmd, uiSeq, oMsgBody));
    }
}

bool ActorSender::SendOriented(Actor* pActor, const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    ChannelOption stOption;
    stOption.bPipeline = true;
    (const_cast<MsgBody&>(oMsgBody)).set_trace_id(pActor->GetTraceId());
    if (oMsgBody.has_req_target())
    {
        if (0 != oMsgBody.req_target().route_id())
        {
            return(SendOriented(pActor, strNodeType, oMsgBody.req_target().route_id(), iCmd, uiSeq, oMsgBody, eCodecType));
        }
        else if (oMsgBody.req_target().route().length() > 0)
        {
            if (eCodecType == CODEC_NEBULA)
            {
                return(IO<CodecNebula>::SendOriented(pActor, strNodeType, oMsgBody.req_target().route_id(), stOption, iCmd, uiSeq, oMsgBody));
            }
            else if (eCodecType == CODEC_NEBULA_IN_NODE)
            {
                return(IO<CodecNebulaInNode>::SendOriented(pActor, strNodeType, oMsgBody.req_target().route_id(), stOption, iCmd, uiSeq, oMsgBody));
            }
            else
            {
                return(IO<CodecProto>::SendOriented(pActor, strNodeType, oMsgBody.req_target().route_id(), stOption, iCmd, uiSeq, oMsgBody));
            }
        }
        else
        {
            return(SendRoundRobin(pActor, strNodeType, iCmd, uiSeq, oMsgBody, eCodecType));
        }
    }
    else
    {
        return(false);
    }
}

bool ActorSender::SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel,
            uint32 uiStreamId, const std::string& strGrpcResponse,
            E_GRPC_STATUS_CODE eStatus, const std::string& strStatusMessage,
            E_COMPRESSION eCompression)
{
    HttpMsg oHttpMsg;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_http_major(2);
    oHttpMsg.set_http_minor(0);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_stream_id(uiStreamId);
    oHttpMsg.mutable_headers()->insert({"content-type", "application/grpc"});
    oHttpMsg.add_adding_never_index_headers("grpc-status");
    oHttpMsg.add_adding_never_index_headers("grpc-message");
    //oHttpMsg.add_adding_never_index_headers("x-trace-id");
    if (eStatus == GRPC_OK)
    {
        uint8 ucCompressedFlag = 0;
        uint32 uiMessageLength = 0;
        std::string strResponseData;
        switch (eCompression)
        {
            case COMPRESS_GZIP:
                if (CodecUtil::Gzip(strGrpcResponse, strResponseData))
                {
                    oHttpMsg.mutable_headers()->insert({"grpc-encoding", "gzip"});
                    ucCompressedFlag = 1;
                    uiMessageLength = strResponseData.size();
                    oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                    oHttpMsg.mutable_body()->append(strResponseData);
                }
                else
                {
                    uiMessageLength = strGrpcResponse.size();
                    oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                    oHttpMsg.mutable_body()->append(strGrpcResponse);
                }
                break;
            default:
                uiMessageLength = strGrpcResponse.size();
                oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                oHttpMsg.mutable_body()->append(strGrpcResponse);
        }
    }
    auto pHeader = oHttpMsg.add_trailer_header();
    pHeader->set_name("grpc-status");
    pHeader->set_value(std::to_string(eStatus));
    pHeader = oHttpMsg.add_trailer_header();
    pHeader->set_name("grpc-message");
    pHeader->set_value(strStatusMessage);
    return(IO<CodecHttp2>::SendResponse(pActor, pChannel, oHttpMsg));
}

bool ActorSender::SendTo(Actor* pActor, const std::string& strUrl, const std::string& strGrpcRequest, E_COMPRESSION eCompression)
{
    int iPort = 0;
    std::string strHost;
    std::string strPath;
    HttpMsg oHttpMsg;
    struct http_parser_url stUrl;
    oHttpMsg.set_http_major(2);
    oHttpMsg.set_http_minor(0);
    oHttpMsg.set_type(HTTP_REQUEST);
    oHttpMsg.set_method(HTTP_POST);
    oHttpMsg.set_url(strUrl);
    oHttpMsg.mutable_headers()->insert({"content-type", "application/grpc"});
    oHttpMsg.mutable_headers()->insert({"te", "trailers"});
    oHttpMsg.add_adding_never_index_headers("x-trace-id");
    if(0 == http_parser_parse_url(strUrl.c_str(), strUrl.length(), 0, &stUrl))
    {
        if(stUrl.field_set & (1 << UF_PORT))
        {
            iPort = stUrl.port;
        }
        else
        {
            iPort = 80;
        }

        if(stUrl.field_set & (1 << UF_HOST) )
        {
            strHost = oHttpMsg.url().substr(stUrl.field_data[UF_HOST].off, stUrl.field_data[UF_HOST].len);
        }

        if (iPort == 80)
        {
            std::string strSchema = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(':'));
            std::transform(strSchema.begin(), strSchema.end(), strSchema.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
            if (strSchema == std::string("https"))
            {
                iPort = 443;
            }
        }

        uint8 ucCompressedFlag = 0;
        uint32 uiMessageLength = 0;
        std::string strRequestData;
        switch (eCompression)
        {
            case COMPRESS_GZIP:
                if (CodecUtil::Gzip(strGrpcRequest, strRequestData))
                {
                    oHttpMsg.mutable_headers()->insert({"grpc-encoding", "gzip"});
                    ucCompressedFlag = 1;
                    uiMessageLength = strRequestData.size();
                    oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                    oHttpMsg.mutable_body()->append(strRequestData);
                }
                else
                {
                    uiMessageLength = strGrpcRequest.size();
                    oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                    oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                    oHttpMsg.mutable_body()->append(strGrpcRequest);
                }
                break;
            default:
                uiMessageLength = strGrpcRequest.size();
                oHttpMsg.mutable_body()->append(1, ucCompressedFlag);
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 24) & 0xFF));
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 16) & 0xFF));
                oHttpMsg.mutable_body()->append(1, ((uiMessageLength >> 8) & 0xFF));
                oHttpMsg.mutable_body()->append(1, (uiMessageLength & 0xFF));
                oHttpMsg.mutable_body()->append(strGrpcRequest);
        }
        return(SendTo(pActor, strHost, iPort, oHttpMsg));
    }
    else
    {
        return(false);
    }
}

bool ActorSender::SendRoundRobin(Actor* pActor, const std::string& strIdentify, const CassMessage& oCassMsg, bool bWithSsl, bool bPipeline)
{
    ChannelOption stOption;
    stOption.bPipeline = bPipeline;
    stOption.bWithSsl = bWithSsl;
    return(IO<CodecCass>::SendRoundRobin(pActor, strIdentify, stOption, oCassMsg));
}

} /* namespace neb */

