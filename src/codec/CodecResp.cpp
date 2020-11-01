/*******************************************************************************
 * Project:  Nebula
 * @file     CodecResp.cpp
 * @brief
 * @author   nebim
 * @date:    2020-10-02
 * @note
 * Modify history:
 ******************************************************************************/
#include "CodecResp.hpp"
#include "util/StringConverter.hpp"
#include "actor/step/RedisStep.hpp" // TODO move to dispatcher

namespace neb
{

CodecResp::CodecResp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType)
{
}

CodecResp::~CodecResp()
{
}

E_CODEC_STATUS CodecResp::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    return(CODEC_STATUS_INVALID);
}

E_CODEC_STATUS CodecResp::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    return(CODEC_STATUS_INVALID);
}

E_CODEC_STATUS CodecResp::Encode(std::shared_ptr<RedisStep> pRedisStep, CBuffer* pBuff)
{
    RedisReply oReply;
    oReply.set_type(REDIS_REPLY_ARRAY);
    auto pElement = oReply.add_element();
    pElement->set_type(REDIS_REPLY_STRING);
    pElement->set_str(pRedisStep->GetCmd());
    auto& rArgs = pRedisStep->GetCmdArguments();
    for (size_t i = 0; i < rArgs.size(); ++i)
    {
        pElement = oReply.add_element();
        pElement->set_type(REDIS_REPLY_STRING);
        pElement->set_str(rArgs[i].first);
    }
    return(Encode(oReply, pBuff));
}

E_CODEC_STATUS CodecResp::Encode(const RedisReply& oReply, CBuffer* pBuff)
{
    int iHadWriteLen = 0;
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    switch (oReply.type())
    {
        case REDIS_REPLY_STRING:
            eStatus = EncodeBulkString(oReply, pBuff);
            break;
        case REDIS_REPLY_ARRAY:
            eStatus = EncodeArray(oReply, pBuff);
            break;
        case REDIS_REPLY_STATUS:
            eStatus = EncodeSimpleString(oReply, pBuff);
            break;
        case REDIS_REPLY_INTEGER:
            eStatus = EncodeInteger(oReply, pBuff);
            break;
        case REDIS_REPLY_ERROR:
            eStatus = EncodeError(oReply, pBuff);
            break;
        case REDIS_REPLY_NIL:
            eStatus = EncodeNull(oReply, pBuff);
            break;
        default:
            LOG4_ERROR("invalid redis reply type %d", oReply.type());
            return(CODEC_STATUS_ERR);
    }
    if (CODEC_STATUS_OK != eStatus)
    {
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
    }
    return(eStatus);
}

E_CODEC_STATUS CodecResp::Decode(CBuffer* pBuff, RedisReply& oReply)
{
    char cFirstByte = 0;
    size_t uiReadIndex = pBuff->GetReadIndex();
    pBuff->ReadByte(cFirstByte);
    E_CODEC_STATUS eStatus = CODEC_STATUS_OK;
    switch (cFirstByte)
    {
        case RESP_SIMPLE_STRING:
            eStatus = DecodeSimpleString(pBuff, oReply);
            break;
        case RESP_INTEGER:
            eStatus = DecodeInteger(pBuff, oReply);
            break;
        case RESP_BULK_STRING:
            eStatus = DecodeBulkString(pBuff, oReply);
            break;
        case RESP_ARRAY:
            eStatus = DecodeArray(pBuff, oReply);
            break;
        case RESP_ERROR:
            eStatus = DecodeError(pBuff, oReply);
            break;
        default:
            oReply.set_type(REDIS_REPLY_ERROR);
            oReply.set_integer(REDIS_ERR_PROTOCOL);
            return(CODEC_STATUS_ERR);
    }
    if (CODEC_STATUS_PAUSE == eStatus)
    {
        pBuff->SetReadIndex(uiReadIndex);
    }
    return(eStatus);
}

}

