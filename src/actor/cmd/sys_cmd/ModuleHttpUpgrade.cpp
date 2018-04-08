/*******************************************************************************
 * Project:  Nebula
 * @file     CmdHttpUpgrade.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年9月1日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/cmd/sys_cmd/ModuleHttpUpgrade.hpp>
#include <cryptopp/base64.h>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

namespace neb
{

//std::string ModuleHttpUpgrade::mc_strWebSocketMagicGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

ModuleHttpUpgrade::ModuleHttpUpgrade(const std::string& strModulePath)
    : Module(strModulePath), mc_strWebSocketMagicGuid("258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
{
}

ModuleHttpUpgrade::~ModuleHttpUpgrade()
{
}

bool ModuleHttpUpgrade::AnyMessage(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg)
{
    if (oHttpMsg.has_upgrade() && oHttpMsg.upgrade().is_upgrade())
    {
        if (std::string("websocket") == oHttpMsg.upgrade().protocol())
        {
            return(WebSocket(stCtx, oHttpMsg));
        }
    }
    HttpMsg oOutHttpMsg;
    LOG4_ERROR("the upgrade protocol %s not supported!", oHttpMsg.upgrade().protocol().c_str());
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(400);
    oOutHttpMsg.set_http_major(oHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
    return(false);
}

bool ModuleHttpUpgrade::WebSocket(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg)
{
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(400);
    oOutHttpMsg.set_http_major(oHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
    if (1 == oHttpMsg.http_major() && 1 == oHttpMsg.http_minor()
            && HTTP_GET == (http_method)oHttpMsg.method())
    {
        std::string strSecWebSocketKey;
        int iSecWebSocketVersion;
        for (int i = 0; i < oHttpMsg.headers_size(); ++i)
        {
            if (std::string("Sec-WebSocket-Key") == oHttpMsg.headers(i).header_name())
            {
                strSecWebSocketKey = oHttpMsg.headers(i).header_value();
            }
            else if (std::string("Sec-WebSocket-Version") == oHttpMsg.headers(i).header_name())
            {
                iSecWebSocketVersion = atoi(oHttpMsg.headers(i).header_value().c_str());
            }
        }
        if (13 != iSecWebSocketVersion)
        {
            LOG4_ERROR("invalid Sec-WebSocket-Version %d, the version must be 13!", iSecWebSocketVersion);
            SendTo(stCtx, oOutHttpMsg);
            return(false);
        }

        /*
         * TODO
         * 如果客户端已经有一个到通过主机/host/和端口/port/对标识的远程主机（IP地址）的WebSocket连接，
         * 即使远程主机是已知的另一个名字，客户端必须等待直到连接已建立或由于连接已失败。
         */
        CryptoPP::Base64Decoder oDecoder;
        oDecoder.Put((CryptoPP::byte*)strSecWebSocketKey.data(), strSecWebSocketKey.size() );
        oDecoder.MessageEnd();
//        CryptoPP::word64 size = oDecoder.MaxRetrievable();
//        if(size && size <= SIZE_MAX)
//        {
//            strDecoded.resize(size);
//            oDecoder.Get((byte*)strDecoded.data(), strDecoded.size());
//        }
        if (16 != oDecoder.MaxRetrievable())
        {
            LOG4_ERROR("invalid Sec-WebSocket-Key %s, the key len after base64 decode must be 16!", strSecWebSocketKey.c_str());
            SendTo(stCtx, oOutHttpMsg);
            return(false);
        }

        std::string strSrcAcceptKey = strSecWebSocketKey + mc_strWebSocketMagicGuid;
        std::string strSha1AcceptKey;
        std::string strBase64EncodeAcceptKey;
        CryptoPP::SHA1 oSha1;
        CryptoPP::HashFilter oHashFilter(oSha1);
        oHashFilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(strSha1AcceptKey), false));
        oHashFilter.Put(reinterpret_cast<const unsigned char*>(strSrcAcceptKey.data()), strSrcAcceptKey.size());
        oHashFilter.MessageEnd();
        //CryptoPP::StringSource(strSecWebSocketKey + mc_strWebSocketMagicGuid, true, new CryptoPP::HashFilter(oSha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(strSha1AcceptKey))));
        CryptoPP::Base64Encoder oEncoder;
        oEncoder.Put((CryptoPP::byte*)strSha1AcceptKey.data(), strSha1AcceptKey.size());
        oEncoder.MessageEnd();
        CryptoPP::word64 size = oEncoder.MaxRetrievable();
        if(size)
        {
            strBase64EncodeAcceptKey.resize(size);
            oEncoder.Get((CryptoPP::byte*)strBase64EncodeAcceptKey.data(), strBase64EncodeAcceptKey.size());
        }
        oOutHttpMsg.set_status_code(101);
        HttpMsg::Header* pHeader = oOutHttpMsg.add_headers();
        pHeader->set_header_name("Upgrade");
        pHeader->set_header_value("websocket");
        pHeader = oOutHttpMsg.add_headers();
        pHeader->set_header_name("Connection");
        pHeader->set_header_value("Upgrade");
        pHeader = oOutHttpMsg.add_headers();
        pHeader->set_header_name("Sec-WebSocket-Extensions");
        pHeader->set_header_value("private-extension");
        pHeader = oOutHttpMsg.add_headers();
        pHeader->set_header_name("Sec-WebSocket-Accept");
        pHeader->set_header_value(strBase64EncodeAcceptKey);
        SendTo(stCtx, oOutHttpMsg);
        GetWorkerImpl(this)->SwitchCodec(stCtx, CODEC_WS_EXTEND_JSON);
    }
    else
    {
        LOG4_ERROR("websocket opening handshake failed for HTTP %d.%d and http method %s!",
                oHttpMsg.http_major(), oHttpMsg.http_minor(),
                http_method_str((http_method)oHttpMsg.method()));
    }
    SendTo(stCtx, oOutHttpMsg);
    return(false);
}

} /* namespace neb */
