/*******************************************************************************
 * Project:  Nebula
 * @file     CodecResp.hpp
 * @brief
 * @author   nebim
 * @date:    2020-10-02
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECRESP_HPP_
#define SRC_CODEC_CODECRESP_HPP_

#include "Codec.hpp"
#include "pb/redis.pb.h"

namespace neb
{

const char RESP_SIMPLE_STRING = '+';
const char RESP_ERROR = '-';
const char RESP_INTEGER = ':';
const char RESP_BULK_STRING = '$';
const char RESP_ARRAY = '*';

class RedisStep;

class CodecResp: public neb::Codec
{
public:
    CodecResp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);
    virtual ~CodecResp();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);

    virtual E_CODEC_STATUS Encode(const RedisReply& oReply, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, RedisReply& oReply);

protected:
    E_CODEC_STATUS EncodeSimpleString(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeError(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeInteger(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeBulkString(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeArray(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeNull(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS DecodeSimpleString(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeError(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeInteger(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeBulkString(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeArray(CBuffer* pBuff, RedisReply& oReply);
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECRESP_HPP_ */

