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

namespace neb
{

class CodecHttp: public Codec
{
public:
    CodecHttp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, ev_tstamp dKeepAlive = 10.0);
    virtual ~CodecHttp();

    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);

    virtual E_CODEC_STATUS Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff);
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, HttpMsg& oHttpMsg);

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

private:
    bool m_bChannelIsClient;    // 当前编解码器所在channel是作为http客户端还是作为http服务端
    uint32 m_uiEncodedNum;
    uint32 m_uiDecodedNum;
    int32 m_iHttpMajor;
    int32 m_iHttpMinor;
    ev_tstamp m_dKeepAlive;
    http_parser_settings m_parser_setting;
    http_parser m_parser;
    HttpMsg m_oParsingHttpMsg;      // TODO 如果是较大的http包只解了一部分，要记录断点位置，收到信的数据再从断点位置开始解
    std::string m_strHttpString;
    std::unordered_map<std::string, std::string> m_mapAddingHttpHeader;       ///< encode前添加的http头，encode之后要清空
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECHTTP_HPP_ */
