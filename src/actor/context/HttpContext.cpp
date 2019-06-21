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
        const HttpMsg& oHttpMsg)
    : Context(pChannel),
      m_oHttpMsg(oHttpMsg)
{
}

HttpContext::~HttpContext()
{
}

bool HttpContext::Response(const std::string& strData)
{
    HttpMsg oHttpMsg = m_oHttpMsg;
    oHttpMsg.set_body(strData);
    if (SendTo(GetChannel(), oHttpMsg))
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
