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
#include "ios/Dispatcher.hpp"

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

bool ModuleHttpUpgrade::AnyMessage(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    if (oHttpMsg.has_upgrade() && oHttpMsg.upgrade().is_upgrade())
    {
        if (std::string("websocket") == oHttpMsg.upgrade().protocol())
        {
            return(WebSocket(pChannel, oHttpMsg));
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

bool ModuleHttpUpgrade::WebSocket(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
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
        int iSecWebSocketVersion = 0;
        auto it = oHttpMsg.headers().find("Sec-WebSocket-Key");
        if (it != oHttpMsg.headers().end())
        {
            strSecWebSocketKey = it->second;
        }
        it = oHttpMsg.headers().find("Sec-WebSocket-Version");
        if (it != oHttpMsg.headers().end())
        {
            iSecWebSocketVersion = atoi(it->second.c_str());
        }
        if (13 != iSecWebSocketVersion)
        {
            LOG4_ERROR("invalid Sec-WebSocket-Version %d, the version must be 13!", iSecWebSocketVersion);
            SendTo(pChannel, oOutHttpMsg);
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
            SendTo(pChannel, oOutHttpMsg);
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
        oOutHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("Upgrade", "websocket"));
        oOutHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("Connection", "Upgrade"));
        oOutHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("Sec-WebSocket-Extensions", "private-extension"));
        oOutHttpMsg.mutable_headers()->insert(google::protobuf::MapPair<std::string, std::string>("Sec-WebSocket-Accept", strBase64EncodeAcceptKey));
        SendTo(pChannel, oOutHttpMsg);
        GetLabor(this)->GetDispatcher()->SwitchCodec(pChannel, CODEC_WS_EXTEND_JSON);
    }
    else
    {
        LOG4_ERROR("websocket opening handshake failed for HTTP %d.%d and http method %s!",
                oHttpMsg.http_major(), oHttpMsg.http_minor(),
                http_method_str((http_method)oHttpMsg.method()));
    }
    SendTo(pChannel, oOutHttpMsg);
    return(false);
}

} /* namespace neb */
