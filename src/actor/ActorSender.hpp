/*******************************************************************************
 * Project:  Nebula
 * @file     ActorSender.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-02-13
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_ACTORSENDER_HPP_
#define SRC_ACTOR_ACTORSENDER_HPP_

#include <memory>
#include "Definition.hpp"
#include "pb/msg.pb.h"
#include "pb/http.pb.h"
#include "pb/redis.pb.h"
#include "channel/Channel.hpp"
#include "codec/Codec.hpp"
#include "codec/CodecUtil.hpp"
#include "codec/grpc/Grpc.hpp"

namespace neb
{

typedef RedisReply RedisMsg;

class Actor;
class SocketChannel;

/**
 * @brief Actor发送代理
 * @note 随着Nebula支持的通信协议的增加，Actor类的Send函数越来越多，Actor的可读性会
 * 变差，也不利于维护，故将独立到ActorSender中来，为保持兼容性，原Actor中的Send函数保留。
 */
class ActorSender
{
public:
    ActorSender();
    virtual ~ActorSender();

    static void SetAuth(Actor* pActor, const std::string& strNodeType, const std::string& strAuth, const std::string& strPassword);

    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel);

    // send pb message
    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    static bool SendTo(Actor* pActor, const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);
    static bool SendTo(Actor* pActor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    static bool SendRoundRobin(Actor* pActor, const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);
    static bool SendOriented(Actor* pActor, const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);
    static bool SendOriented(Actor* pActor, const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);

    // send http message
    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);
    static bool SendTo(Actor* pActor, const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq = 0);

    // send redis message
    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const RedisReply& oRedisReply);
    static bool SendTo(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = true, uint32 uiStepSeq = 0);
    static bool SendRoundRobin(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = false);
    static bool SendToCluster(Actor* pActor, const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = true, bool bEnableReadOnly = false);

    // send raw message
    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel, const char* pRawData, uint32 uiRawDataSize);
    static bool SendTo(Actor* pActor, const std::string& strIdentify, const char* pRawData, uint32 uiRawDataSize, bool bWithSsl = false, bool bPipeline = false, uint32 uiStepSeq = 0);

    // send grpc message
    static bool SendTo(Actor* pActor, std::shared_ptr<SocketChannel> pChannel,
            uint32 uiStreamId, const std::string& strGrpcResponse,
            E_GRPC_STATUS_CODE eStatus, const std::string& strStatusMessage,
            E_COMPRESSION eCompression = COMPRESS_NA);
    static bool SendTo(Actor* pActor, const std::string& strUrl, const std::string& strGrpcRequest, E_COMPRESSION eCompression = COMPRESS_NA);
};

} /* namespace neb */

#endif

