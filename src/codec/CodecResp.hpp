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

#include <memory>
#include "Codec.hpp"
#include "pb/redis.pb.h"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"

namespace neb
{

class RedisStep;

struct CodecRespReadTask
{
    int iType;
    int iElements;                  ///< number of elements in multibulk container
    int iIndex;                     ///< index in parent (array) object
    RedisReply* pReply;             ///<  holds user-generated value for a read task
    CodecRespReadTask *pParent;     ///<  parent task

    CodecRespReadTask()
        : iType(-1), iElements(-1), iIndex(-1), pReply(nullptr), pParent(nullptr)
    {
    }

    void Reset()
    {
        iType = -1;
        iElements = -1;
        iIndex = -1;
        pReply = nullptr;
        pParent = nullptr;
    }
};

class CodecResp: public neb::Codec
{
public:
    CodecResp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecResp();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_RESP);
    }

    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const RedisReply& oReply);

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const RedisReply& oReply);

    virtual bool DecodeWithStack() const
    {
        return(m_bDecodeWithStack);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const RedisReply& oReply, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS Decode(CBuffer* pBuff, RedisReply& oReply, CBuffer* pReactBuff);

protected:
    E_CODEC_STATUS EncodeSimpleString(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeError(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeInteger(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeBulkString(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeArray(const RedisReply& oReply, CBuffer* pBuff);
    E_CODEC_STATUS EncodeNull(const RedisReply& oReply, CBuffer* pBuff);
    
    E_CODEC_STATUS DecodeRecursive(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeSimpleString(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeError(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeInteger(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeBulkString(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeArray(CBuffer* pBuff, RedisReply& oReply);

    E_CODEC_STATUS DecodeWithStack(CBuffer* pBuff, RedisReply& oReply);
    E_CODEC_STATUS DecodeWithStack(CBuffer* pBuff);
    E_CODEC_STATUS DecodeLineItem(CBuffer* pBuff);
    E_CODEC_STATUS DecodeBulkItem(CBuffer* pBuff);
    E_CODEC_STATUS DecodeArray(CBuffer* pBuff);
    void MoveToNextTask();
    RedisReply* NewReply(CodecRespReadTask *pParent);

private:
    bool m_bDecodeWithStack = false;
    unsigned int m_uiDecodedNum = 0;
    unsigned int m_uiIncompleteDecodeNum = 0;
    int m_iErrno    = 0;               /* Error flags, 0 when there is no error */
    char m_szErrMsg[128] = {0};       /* String representation of error when applicable */
    int m_iCurrentReadTaskIndex = -1;
    CodecRespReadTask m_oReadStack[9];
    RedisReply m_oReply;                 ///< Temporary reply pointer

    static const char RESP_SIMPLE_STRING;
    static const char RESP_ERROR;
    static const char RESP_INTEGER;
    static const char RESP_BULK_STRING;
    static const char RESP_ARRAY;
};

// request
template<typename ...Targs>
int CodecResp::Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const RedisReply& oReply)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<RedisReply>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(Type(), uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<RedisReply>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, Type());
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(const_cast<RedisReply&>(oReply)));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(Type(), uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<RedisReply>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(const_cast<RedisReply&>(oReply)));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

// response
template<typename ...Targs>
int CodecResp::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const RedisReply& oReply)
{
    uint32 uiFrom;
    uint32 uiTo;
    std::static_pointer_cast<SpecChannel<RedisReply>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiTo, uiFrom, uiFlags, uiStepSeq, oReply));
}

} /* namespace neb */

#endif /* SRC_CODEC_CODECRESP_HPP_ */

