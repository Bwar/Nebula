/*******************************************************************************
 * Project:  Nebula
 * @file     CodecHttp.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/
#include <algorithm>
#include "util/StringCoder.hpp"
#include "logger/NetLogger.hpp"
#include "CodecHttp.hpp"

#define STATUS_CODE(code, str) case code: return str;

static const char * status_string(int code)
{
    switch (code)
    {
        STATUS_CODE(100, "Continue")
        STATUS_CODE(101, "Switching Protocols")
        STATUS_CODE(102, "Processing")            // RFC 2518) obsoleted by RFC 4918
        STATUS_CODE(200, "OK")
        STATUS_CODE(201, "Created")
        STATUS_CODE(202, "Accepted")
        STATUS_CODE(203, "Non-Authoritative Information")
        STATUS_CODE(204, "No Content")
        STATUS_CODE(205, "Reset Content")
        STATUS_CODE(206, "Partial Content")
        STATUS_CODE(207, "Multi-Status")               // RFC 4918
        STATUS_CODE(300, "Multiple Choices")
        STATUS_CODE(301, "Moved Permanently")
        STATUS_CODE(302, "Moved Temporarily")
        STATUS_CODE(303, "See Other")
        STATUS_CODE(304, "Not Modified")
        STATUS_CODE(305, "Use Proxy")
        STATUS_CODE(307, "Temporary Redirect")
        STATUS_CODE(400, "Bad Request")
        STATUS_CODE(401, "Unauthorized")
        STATUS_CODE(402, "Payment Required")
        STATUS_CODE(403, "Forbidden")
        STATUS_CODE(404, "Not Found")
        STATUS_CODE(405, "Method Not Allowed")
        STATUS_CODE(406, "Not Acceptable")
        STATUS_CODE(407, "Proxy Authentication Required")
        STATUS_CODE(408, "Request Time-out")
        STATUS_CODE(409, "Conflict")
        STATUS_CODE(410, "Gone")
        STATUS_CODE(411, "Length Required")
        STATUS_CODE(412, "Precondition Failed")
        STATUS_CODE(413, "Request Entity Too Large")
        STATUS_CODE(414, "Request-URI Too Large")
        STATUS_CODE(415, "Unsupported Media Type")
        STATUS_CODE(416, "Requested Range Not Satisfiable")
        STATUS_CODE(417, "Expectation Failed")
        STATUS_CODE(418, "I\"m a teapot")              // RFC 2324
        STATUS_CODE(422, "Unprocessable Entity")       // RFC 4918
        STATUS_CODE(423, "Locked")                     // RFC 4918
        STATUS_CODE(424, "Failed Dependency")          // RFC 4918
        STATUS_CODE(425, "Unordered Collection")       // RFC 4918
        STATUS_CODE(426, "Upgrade Required")           // RFC 2817
        STATUS_CODE(500, "Internal Server Error")
        STATUS_CODE(501, "Not Implemented")
        STATUS_CODE(502, "Bad Gateway")
        STATUS_CODE(503, "Service Unavailable")
        STATUS_CODE(504, "Gateway Time-out")
        STATUS_CODE(505, "HTTP Version not supported")
        STATUS_CODE(506, "Variant Also Negotiates")    // RFC 2295
        STATUS_CODE(507, "Insufficient Storage")       // RFC 4918
        STATUS_CODE(509, "Bandwidth Limit Exceeded")
        STATUS_CODE(510, "Not Extended")                // RFC 2774
    }

    return 0;
}

namespace neb
{

CodecHttp::CodecHttp(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, ev_tstamp dKeepAlive)
    : Codec(pLogger, eCodecType),
      m_bChannelIsClient(false), m_uiEncodedNum(0), m_uiDecodedNum(0),
      m_iHttpMajor(1), m_iHttpMinor(1), m_dKeepAlive(dKeepAlive)
{
}

CodecHttp::~CodecHttp()
{
}

E_CODEC_STATUS CodecHttp::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    LOG4_TRACE(" ");
    ++m_uiEncodedNum;
    if (m_uiEncodedNum > m_uiDecodedNum)
    {
        m_bChannelIsClient = true;
    }
    HttpMsg oHttpMsg;
    if (oHttpMsg.ParseFromString(oMsgBody.data()))
    {
        oHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("x-cmd", std::to_string(oMsgHead.cmd())));
        oHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("x-seq", std::to_string(oMsgHead.seq())));
        return(Encode(oHttpMsg, pBuff));
    }
    else
    {
        LOG4_ERROR("oHttpMsg.ParseFromString() error!");
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS CodecHttp::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE(" ");
    ++m_uiDecodedNum;
    if (pBuff->ReadableBytes() == 0)
    {
        LOG4_DEBUG("no data...");
        return(CODEC_STATUS_PAUSE);
    }
    HttpMsg oHttpMsg;
    E_CODEC_STATUS eCodecStatus = Decode(pBuff, oHttpMsg);
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        oMsgBody.set_data(oHttpMsg.body());
        oMsgHead.set_len(oMsgBody.ByteSize());
        for (auto it = oHttpMsg.headers().begin(); it != oHttpMsg.headers().end(); ++it)
        {
            if (std::string("x-cmd") == it->first
                            || std::string("x-CMD") == it->first
                            || std::string("x-Cmd") == it->first)
            {
                oMsgHead.set_cmd(atoi(it->second.c_str()));
            }
            else if (std::string("x-seq") == it->first
                            || std::string("x-SEQ") == it->first
                            || std::string("x-Seq") == it->first)
            {
                oMsgHead.set_seq(strtoul(it->second.c_str(), NULL, 10));
            }
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecHttp::Encode(const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    LOG4_TRACE("pBuff->ReadableBytes() = %u, ReadIndex = %u, WriteIndex = %u",
                    pBuff->ReadableBytes(), pBuff->GetReadIndex(), pBuff->GetWriteIndex());
    ++m_uiEncodedNum;
    if (m_uiEncodedNum > m_uiDecodedNum)
    {
        m_bChannelIsClient = true;
    }
    if (0 == oHttpMsg.http_major())
    {
        LOG4_ERROR("miss http version!");
        m_mapAddingHttpHeader.clear();
        return(CODEC_STATUS_ERR);
    }

    int iWriteSize = 0;
    int iHadEncodedSize = 0;
    if (HTTP_REQUEST == oHttpMsg.type())
    {
        if (oHttpMsg.url().size() == 0)
        {
            LOG4_ERROR("miss url!");
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        int iPort = 0;
        std::string strHost;
        std::string strPath;
        struct http_parser_url stUrl;
        if(0 == http_parser_parse_url(oHttpMsg.url().c_str(), oHttpMsg.url().length(), 0, &stUrl))
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

            if(stUrl.field_set & (1 << UF_PATH))
            {
                strPath = oHttpMsg.url().substr(stUrl.field_data[UF_PATH].off, stUrl.field_data[UF_PATH].len);
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
        }
        else
        {
            LOG4_ERROR("http_parser_parse_url error!");
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        if (stUrl.field_data[UF_PATH].off >= oHttpMsg.url().size())
        {
            LOG4_ERROR("invalid url \"%s\"!", oHttpMsg.url().c_str());
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        iWriteSize = pBuff->Printf("%s %s HTTP/%u.%u\r\n", http_method_str((http_method)oHttpMsg.method()),
                        oHttpMsg.url().substr(stUrl.field_data[UF_PATH].off, std::string::npos).c_str(),
                        oHttpMsg.http_major(), oHttpMsg.http_minor());
        if (iWriteSize < 0)
        {
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        else
        {
            iHadEncodedSize += iWriteSize;
        }
        iWriteSize = pBuff->Printf("Host: %s:%d\r\n", strHost.c_str(), iPort);
        if (iWriteSize < 0)
        {
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        else
        {
            iHadEncodedSize += iWriteSize;
        }
    }
    else if (HTTP_RESPONSE == oHttpMsg.type())
    {
        if (0 == oHttpMsg.status_code())
        {
            LOG4_ERROR("miss status code!");
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        iWriteSize = pBuff->Printf("HTTP/%u.%u %u %s\r\n", m_iHttpMajor, m_iHttpMinor,
                        oHttpMsg.status_code(), status_string(oHttpMsg.status_code()));
        if (iWriteSize < 0)
        {
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        else
        {
            iHadEncodedSize += iWriteSize;
        }
        if (!m_bChannelIsClient)
        {
            if (m_dKeepAlive == 0)
            {
                m_mapAddingHttpHeader.insert(std::pair<std::string, std::string>("Connection", "close"));
            }
            else
            {
                m_mapAddingHttpHeader.insert(std::pair<std::string, std::string>("Connection", "keep-alive"));
            }
        }
        m_mapAddingHttpHeader.insert(std::make_pair("Server", "NebulaHttp"));
        m_mapAddingHttpHeader.insert(std::make_pair("Allow", "POST,GET"));
        //m_mapAddingHttpHeader.insert(std::make_pair("Content-Type", "application/json;charset=UTF-8"));
    }
    bool bIsChunked = false;
    bool bIsGzip = false;   // 是否用gizp压缩传输包
    for (auto h_iter = oHttpMsg.headers().begin(); h_iter != oHttpMsg.headers().end(); ++h_iter)
    {
        auto h_a_iter = m_mapAddingHttpHeader.find(h_iter->first);
        if (h_a_iter == m_mapAddingHttpHeader.end())
        {
            m_mapAddingHttpHeader.insert(std::make_pair(h_iter->first, h_iter->second));
        }
    }
    for (auto h_iter = m_mapAddingHttpHeader.begin(); h_iter != m_mapAddingHttpHeader.end(); ++h_iter)
    {
        iWriteSize = pBuff->Printf("%s: %s\r\n", h_iter->first.c_str(), h_iter->second.c_str());
        if (iWriteSize < 0)
        {
            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            m_mapAddingHttpHeader.clear();
            return(CODEC_STATUS_ERR);
        }
        else
        {
            iHadEncodedSize += iWriteSize;
        }
        if (std::string("Content-Encoding") == h_iter->first && std::string("gzip") == h_iter->second)
        {
            bIsGzip = true;
        }
        if (std::string("Transfer-Encoding") == h_iter->first && std::string("chunked") == h_iter->second)
        {
            bIsChunked = true;
        }
    }
    if (oHttpMsg.body().size() > 0)
    {
        std::string strGzipData;
        if (bIsGzip)
        {
            if (!Gzip(oHttpMsg.body(), strGzipData))
            {
                LOG4_ERROR("gzip error!");
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                m_mapAddingHttpHeader.clear();
                return(CODEC_STATUS_ERR);
            }
        }

        if (bIsChunked)     // Transfer-Encoding: chunked
        {
            if (oHttpMsg.encoding() == 0)
            {
                iWriteSize = pBuff->Printf("\r\n");
                if (iWriteSize < 0)
                {
                    pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                    m_mapAddingHttpHeader.clear();
                    return(CODEC_STATUS_ERR);
                }
                else
                {
                    iHadEncodedSize += iWriteSize;
                }
            }
            else
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                iHadEncodedSize = 0;
            }
            if (strGzipData.size() > 0)
            {
                iWriteSize = pBuff->Printf("%x\r\n", strGzipData.size());
                if (iWriteSize < 0)
                {
                    pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                    m_mapAddingHttpHeader.clear();
                    return(CODEC_STATUS_ERR);
                }
                else
                {
                    iHadEncodedSize += iWriteSize;
                }
                iWriteSize = pBuff->Write(strGzipData.c_str(), strGzipData.size());
                if (iWriteSize < 0)
                {
                    pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                    m_mapAddingHttpHeader.clear();
                    return(CODEC_STATUS_ERR);
                }
                else
                {
                    iHadEncodedSize += iWriteSize;
                }
            }
            else
            {
                iWriteSize = pBuff->Printf("%x\r\n", oHttpMsg.body().size());
                if (iWriteSize < 0)
                {
                    pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                    m_mapAddingHttpHeader.clear();
                    return(CODEC_STATUS_ERR);
                }
                else
                {
                    iHadEncodedSize += iWriteSize;
                }
                if (oHttpMsg.body().size() == 0)
                {
                    pBuff->Printf("\r\n");
                }
                else
                {
                    iWriteSize = pBuff->Write(oHttpMsg.body().c_str(), oHttpMsg.body().size());
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                }
            }
        }
        else    // Content-Length: %u
        {
            if (strGzipData.size() > 0)
            {
                if (strGzipData.size() > 8192)  // 长度太长，使用chunked编码传输
                {
                    bIsChunked = true;
                    iWriteSize = pBuff->Printf("Transfer-Encoding: chunked\r\n\r\n");
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                    size_t iChunkLength = 8192;
                    for (size_t iPos = 0; iPos < strGzipData.size(); iPos += iChunkLength)
                    {
                        iChunkLength = (iChunkLength < strGzipData.size() - iPos) ? iChunkLength : (strGzipData.size() - iPos);
                        iWriteSize = pBuff->Printf("%x\r\n", iChunkLength);
                        if (iWriteSize < 0)
                        {
                            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                            m_mapAddingHttpHeader.clear();
                            return(CODEC_STATUS_ERR);
                        }
                        else
                        {
                            iHadEncodedSize += iWriteSize;
                        }
                        iWriteSize = pBuff->Printf("%s\r\n", strGzipData.substr(iPos, iChunkLength).c_str(), iChunkLength);
                        if (iWriteSize < 0)
                        {
                            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                            m_mapAddingHttpHeader.clear();
                            return(CODEC_STATUS_ERR);
                        }
                        else
                        {
                            iHadEncodedSize += iWriteSize;
                        }
                    }
                    iWriteSize = pBuff->Printf("0\r\n\r\n");
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                }
                else
                {
                    iWriteSize = pBuff->Printf("Content-Length: %u\r\n\r\n", strGzipData.size());
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                    iWriteSize = pBuff->Write(strGzipData.c_str(), strGzipData.size());
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                }
            }
            else
            {
                if (oHttpMsg.body().size() > 8192)
                {
                    bIsChunked = true;
                    iWriteSize = pBuff->Printf("Transfer-Encoding: chunked\r\n\r\n");
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                    size_t iChunkLength = 8192;
                    for (size_t iPos = 0; iPos < oHttpMsg.body().size(); iPos += iChunkLength)
                    {
                        iChunkLength = (iChunkLength < oHttpMsg.body().size() - iPos) ? iChunkLength : (oHttpMsg.body().size() - iPos);
                        iWriteSize = pBuff->Printf("%x\r\n", iChunkLength);
                        if (iWriteSize < 0)
                        {
                            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                            m_mapAddingHttpHeader.clear();
                            return(CODEC_STATUS_ERR);
                        }
                        else
                        {
                            iHadEncodedSize += iWriteSize;
                        }
                        iWriteSize = pBuff->Printf("%s\r\n", oHttpMsg.body().substr(iPos, iChunkLength).c_str(), iChunkLength);
                        if (iWriteSize < 0)
                        {
                            pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                            m_mapAddingHttpHeader.clear();
                            return(CODEC_STATUS_ERR);
                        }
                        else
                        {
                            iHadEncodedSize += iWriteSize;
                        }
                    }
                    iWriteSize = pBuff->Printf("0\r\n\r\n");
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                }
                else
                {
                    iWriteSize = pBuff->Printf("Content-Length: %u\r\n\r\n", oHttpMsg.body().size());
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                    iWriteSize = pBuff->Write(oHttpMsg.body().c_str(), oHttpMsg.body().size());
                    if (iWriteSize < 0)
                    {
                        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                        m_mapAddingHttpHeader.clear();
                        return(CODEC_STATUS_ERR);
                    }
                    else
                    {
                        iHadEncodedSize += iWriteSize;
                    }
                }
            }
        }
    }
    else
    {
        if (bIsChunked)
        {
            if (oHttpMsg.encoding() == 0)
            {
                iWriteSize = pBuff->Printf("\r\n");
                if (iWriteSize < 0)
                {
                    pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                    m_mapAddingHttpHeader.clear();
                    return(CODEC_STATUS_ERR);
                }
                else
                {
                    iHadEncodedSize += iWriteSize;
                }
            }
            else
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
            }
            iWriteSize = pBuff->Printf("0\r\n\r\n");
            if (iWriteSize < 0)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                m_mapAddingHttpHeader.clear();
                return(CODEC_STATUS_ERR);
            }
            else
            {
                iHadEncodedSize += iWriteSize;
            }
        }
        else
        {
            iWriteSize = pBuff->Printf("Content-Length: 0\r\n\r\n");
            if (iWriteSize < 0)
            {
                pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadEncodedSize);
                m_mapAddingHttpHeader.clear();
                return(CODEC_STATUS_ERR);
            }
            else
            {
                iHadEncodedSize += iWriteSize;
            }
        }
    }
    iWriteSize = pBuff->WriteByte('\0');
    size_t iWriteIndex = pBuff->GetWriteIndex();
    LOG4_TRACE("%s", pBuff->GetRawReadBuffer());
    pBuff->SetWriteIndex(iWriteIndex - iWriteSize);
    LOG4_TRACE("pBuff->ReadableBytes() = %u, ReadIndex = %u, WriteIndex = %u, iHadEncodedSize = %d",
                    pBuff->ReadableBytes(), pBuff->GetReadIndex(), pBuff->GetWriteIndex(), iHadEncodedSize);
    m_mapAddingHttpHeader.clear();
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecHttp::Decode(CBuffer* pBuff, HttpMsg& oHttpMsg)
{
    LOG4_TRACE(" ");
    ++m_uiDecodedNum;
    if (pBuff->ReadableBytes() == 0)
    {
        if (CloseRightAway())
        {
            return(CODEC_STATUS_EOF);
        }
        return(CODEC_STATUS_PAUSE);
    }
    m_parser_setting.on_message_begin = OnMessageBegin;
    m_parser_setting.on_url = OnUrl;
    m_parser_setting.on_status = OnStatus;
    m_parser_setting.on_header_field = OnHeaderField;
    m_parser_setting.on_header_value = OnHeaderValue;
    m_parser_setting.on_headers_complete = OnHeadersComplete;
    m_parser_setting.on_body = OnBody;
    m_parser_setting.on_message_complete = OnMessageComplete;
    m_parser_setting.on_chunk_header = OnChunkHeader;
    m_parser_setting.on_chunk_complete = OnChunkComplete;
    m_parser.data = &oHttpMsg;
    http_parser_init(&m_parser, HTTP_BOTH);
    const char* pDecodeBuff = pBuff->GetRawReadBuffer();
    size_t uiDecodeBuffLen = pBuff->ReadableBytes();
    size_t uiLen = http_parser_execute(&m_parser, &m_parser_setting,
                    pDecodeBuff, uiDecodeBuffLen);
    if (!oHttpMsg.is_decoding())
    {
        if(m_parser.http_errno != HPE_OK)
        {
            LOG4_ERROR("Failed to parse http message for cause:%s",
                            http_errno_name((http_errno)m_parser.http_errno));
            return(CODEC_STATUS_ERR);
        }
        pBuff->AdvanceReadIndex(uiLen);
    }
    else
    {
        LOG4_TRACE("decoding...");
        return(CODEC_STATUS_PAUSE);
    }
    if (HTTP_REQUEST == oHttpMsg.type())
    {
        m_iHttpMajor = oHttpMsg.http_major();
        m_iHttpMinor = oHttpMsg.http_minor();
        m_dKeepAlive = oHttpMsg.keep_alive();
    }
    else
    {
        if (m_bChannelIsClient)
        {
            if (oHttpMsg.keep_alive() == 0.0)
            {
                m_dKeepAlive = 0.0;
            }
        }
    }
    auto iter = oHttpMsg.headers().find("Content-Encoding");
    if (iter != oHttpMsg.headers().end())
    {
        if ("gzip" == iter->second)
        {
            std::string strData;
            if (Gunzip(oHttpMsg.body(), strData))
            {
                oHttpMsg.set_body(strData);
            }
            else
            {
                LOG4_ERROR("guzip error!");
                return(CODEC_STATUS_ERR);
            }
        }
    }
    LOG4_DEBUG("%s", ToString(oHttpMsg).c_str());
    return(CODEC_STATUS_OK);
}

void CodecHttp::AddHttpHeader(const std::string& strHeaderName, const std::string& strHeaderValue)
{
    m_mapAddingHttpHeader.insert(std::pair<std::string, std::string>(strHeaderName, strHeaderValue));
}

const std::string& CodecHttp::ToString(const HttpMsg& oHttpMsg)
{
    LOG4_TRACE(" ");
    m_strHttpString.clear();
    char prover[16];
    sprintf(prover, "HTTP/%u.%u", oHttpMsg.http_major(), oHttpMsg.http_minor());

    if (HTTP_REQUEST == oHttpMsg.type())
    {
        m_strHttpString += http_method_str((http_method)oHttpMsg.method());
        m_strHttpString += " ";
        m_strHttpString += oHttpMsg.url();
        m_strHttpString += " ";
        m_strHttpString += prover;
        m_strHttpString += "\r\n";
    }
    else
    {
        m_strHttpString += prover;
        if (oHttpMsg.status_code() > 0)
        {
            char tmp[10];
            sprintf(tmp, " %u\r\n", oHttpMsg.status_code());
            m_strHttpString += tmp;
        }
    }
    for (auto it = oHttpMsg.headers().begin(); it != oHttpMsg.headers().end(); ++it)
    {
        m_strHttpString += it->first;
        m_strHttpString += ":";
        m_strHttpString += it->second;
        m_strHttpString += "\r\n";
    }
    m_strHttpString += "\r\n";

    if (oHttpMsg.body().size() > 0)
    {
        m_strHttpString += oHttpMsg.body();
        m_strHttpString += "\r\n\r\n";
    }
    return(m_strHttpString);
}

bool CodecHttp::CloseRightAway() const
{
    if (m_bChannelIsClient)
    {
        if (m_dKeepAlive == 0.0)
        {
            return(true);
        }
        else
        {
            return(false);
        }
    }
    else
    {
        return(false);
    }
}

int CodecHttp::OnMessageBegin(http_parser *parser)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    pHttpMsg->set_is_decoding(true);
    return(0);
}

int CodecHttp::OnUrl(http_parser *parser, const char *at, size_t len)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    pHttpMsg->set_url(at, len);
    struct http_parser_url stUrl;
    if(0 == http_parser_parse_url(at, len, 0, &stUrl))
    {
//        if(stUrl.field_set & (1 << UF_PORT))
//        {
//            pHttpMsg->set_port(stUrl.port);
//        }
//        else
//        {
//            pHttpMsg->set_port(80);
//        }
//
//        if(stUrl.field_set & (1 << UF_HOST) )
//        {
//            char* host = (char*)malloc(stUrl.field_data[UF_HOST].len+1);
//            strncpy(host, at+stUrl.field_data[UF_HOST].off, stUrl.field_data[UF_HOST].len);
//            host[stUrl.field_data[UF_HOST].len] = 0;
//            pHttpMsg->set_host(host, stUrl.field_data[UF_HOST].len+1);
//            free(host);
//        }

        if(stUrl.field_set & (1 << UF_PATH))
        {
            char *path = (char*)malloc(stUrl.field_data[UF_PATH].len);
            strncpy(path, at+stUrl.field_data[UF_PATH].off, stUrl.field_data[UF_PATH].len);
            pHttpMsg->set_path(path, stUrl.field_data[UF_PATH].len);
            free(path);
        }

        if (stUrl.field_set & (1 << UF_QUERY))
        {
            std::string strQuery;
            strQuery.assign(at+stUrl.field_data[UF_QUERY].off, stUrl.field_data[UF_QUERY].len);
            std::map<std::string, std::string> mapParam;
            DecodeParameter(strQuery, mapParam);
            for (auto it = mapParam.begin(); it != mapParam.end(); ++it)
            {
                (*pHttpMsg->mutable_params())[it->first] = it->second;
            }
        }
    }
    return 0;
}

int CodecHttp::OnStatus(http_parser *parser, const char *at, size_t len)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    pHttpMsg->set_status_code(parser->status_code);
    return(0);
}

int CodecHttp::OnHeaderField(http_parser *parser, const char *at, size_t len)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    pHttpMsg->set_body(at, len);        // 用body暂存head_name，解析完head_value后再填充到head里
    return(0);
}

int CodecHttp::OnHeaderValue(http_parser *parser, const char *at, size_t len)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    std::string strHeadName = pHttpMsg->body();
    std::string strHeadValue;
    strHeadValue.assign(at, len);
    pHttpMsg->set_body("");
    pHttpMsg->mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>(strHeadName, strHeadValue));
    if (strHeadName == std::string("Keep-Alive"))
    {
        pHttpMsg->set_keep_alive(atof(strHeadValue.c_str()));
    }
    else if (std::string("Connection") == strHeadName)
    {
        if (std::string("keep-alive") == strHeadValue)
        {
            pHttpMsg->set_keep_alive(-1);     // 由配置的IoTimeout决定
        }
        else if (std::string("close") == strHeadValue)
        {
            pHttpMsg->set_keep_alive(0.0);
        }

        if (std::string("Upgrade") == strHeadValue)
        {
            pHttpMsg->mutable_upgrade()->set_is_upgrade(true);
        }
        else
        {
            pHttpMsg->mutable_upgrade()->set_is_upgrade(false);
        }
    }
    else if (std::string("Upgrade") == strHeadName)
    {
        pHttpMsg->mutable_upgrade()->set_protocol(strHeadValue);
    }
    return(0);
}

int CodecHttp::OnHeadersComplete(http_parser *parser)
{
    return(0);
}

int CodecHttp::OnBody(http_parser *parser, const char *at, size_t len)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    if (pHttpMsg->body().size() > 0)
    {
        pHttpMsg->mutable_body()->append(at, len);
    }
    else
    {
        pHttpMsg->set_body(at, len);
    }
    return(0);
}

int CodecHttp::OnMessageComplete(http_parser *parser)
{
    HttpMsg* pHttpMsg = (HttpMsg*) parser->data;
    if (0 != parser->status_code)
    {
        pHttpMsg->set_status_code(parser->status_code);
        pHttpMsg->set_type(HTTP_RESPONSE);
    }
    else
    {
        pHttpMsg->set_method(parser->method);
        pHttpMsg->set_type(HTTP_REQUEST);
    }
    pHttpMsg->set_http_major(parser->http_major);
    pHttpMsg->set_http_minor(parser->http_minor);
    pHttpMsg->set_is_decoding(false);

    if (!http_should_keep_alive(parser))
    {
        pHttpMsg->set_keep_alive(0.0); 
    }
    /*
    switch ((http_method)pHttpMsg->method())
    {
        case HTTP_GET:
        case HTTP_OPTIONS:
            pHttpMsg->set_is_decoding(false);
            break;
        default:
            if (parser->content_length == 0)
            {
                pHttpMsg->set_is_decoding(false);
            }
            else if (parser->content_length == pHttpMsg->body().size())
            {
                pHttpMsg->set_is_decoding(false);
            }
            break;
    }
    */
    return(0);
}

int CodecHttp::OnChunkHeader(http_parser *parser)
{
    return(0);
}

int CodecHttp::OnChunkComplete(http_parser *parser)
{
    return(0);
}


} /* namespace neb */
