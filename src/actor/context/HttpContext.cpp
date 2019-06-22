/*******************************************************************************
 * Project:  Nebula
 * @file     HttpContext.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/

#include "HttpContext.hpp"
#include "util/http/http_parser.h"

namespace neb
{

HttpContext::HttpContext()
{
}

HttpContext::HttpContext(
        std::shared_ptr<SocketChannel> pChannel)
    : Context(pChannel)
{
}

HttpContext::HttpContext(
        std::shared_ptr<SocketChannel> pChannel,
        const HttpMsg& oReqHttpMsg)
    : Context(pChannel),
      m_oReqHttpMsg(oReqHttpMsg)
{
}

HttpContext::~HttpContext()
{
}

bool HttpContext::Response(const std::string& strData)
{
    HttpMsg oRspHttpMsg;
    oRspHttpMsg.set_type(HTTP_RESPONSE);
    oRspHttpMsg.set_status_code(200);
    oRspHttpMsg.set_http_major(m_oReqHttpMsg.http_major());
    oRspHttpMsg.set_http_major(m_oReqHttpMsg.http_minor());
    oRspHttpMsg.set_body(strData);
    if (SendTo(GetChannel(), oRspHttpMsg))
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
