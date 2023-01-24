/*******************************************************************************
 * Project:  Nebula
 * @file     CodecHttp.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECHTTP_HPP_
#define SRC_CODEC_CODECHTTP_HPP_

#include "util/http/http_parser.h"
#include "pb/http.pb.h"
#include "Codec.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"

namespace neb
{

class CodecHttp: public Codec
{
public:
    CodecHttp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel, ev_tstamp dKeepAlive = -1.0);
    virtual ~CodecHttp();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_HTTP);
    }
    
    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg);

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg);

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff);
    E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg);
    E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg, CBuffer* pReactBuff);

    /**
     * @brief 添加http头
     * @note 在encode前，允许框架根据连接属性添加http头
     */
    virtual void AddHttpHeader(const std::string& strHeaderName, const std::string& strHeaderValue);

    const std::string& ToString(const HttpMsg& oHttpMsg);

public:
    ev_tstamp GetKeepAlive() const
    {
        return(m_dKeepAlive);
    }

    bool CloseRightAway() const;

protected:
    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, size_t len);
    static int OnStatus(http_parser *parser, const char *at, size_t len);
    static int OnHeaderField(http_parser *parser, const char *at, size_t len);
    static int OnHeaderValue(http_parser *parser, const char *at, size_t len);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, size_t len);
    static int OnMessageComplete(http_parser *parser);
    static int OnChunkHeader(http_parser *parser);
    static int OnChunkComplete(http_parser *parser);

    HttpMsg* MutableParsingHttpMsg()
    {
        return(&m_oParsingHttpMsg);
    }

private:
    bool m_bIsDecoding;         // 是否編解碼完成
    int32 m_iHttpMajor;
    int32 m_iHttpMinor;
    ev_tstamp m_dKeepAlive;
    http_parser_settings m_parser_setting;
    http_parser m_parser;
    HttpMsg m_oParsingHttpMsg;      // TODO 如果是较大的http包只解了一部分，要记录断点位置，收到信的数据再从断点位置开始解
    std::string m_strHttpString;
    std::unordered_map<std::string, std::string> m_mapAddingHttpHeader;       ///< encode前添加的http头，encode之后要清空
};

// request
template<typename ...Targs>
int CodecHttp::Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<HttpMsg>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(Type(), uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<HttpMsg>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, Type());
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(const_cast<HttpMsg&>(oHttpMsg)));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(Type(), uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<HttpMsg>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(const_cast<HttpMsg&>(oHttpMsg)));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

// response
template<typename ...Targs>
int CodecHttp::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, const HttpMsg& oHttpMsg)
{
    uint32 uiFrom;
    uint32 uiTo;
    std::static_pointer_cast<SpecChannel<HttpMsg>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiTo, uiFrom, uiFlags, uiStepSeq, oHttpMsg));
}

} /* namespace neb */

#endif /* SRC_CODEC_CODECHTTP_HPP_ */
