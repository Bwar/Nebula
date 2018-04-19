/*******************************************************************************
 * Project:  Nebula
 * @file     HttpStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#include <actor/step/HttpStep.hpp>

namespace neb
{

HttpStep::HttpStep(Step* pNextStep, ev_tstamp dTimeout)
    : Step(ACT_HTTP_STEP, pNextStep, dTimeout)
{
}

HttpStep::HttpStep(std::shared_ptr<SocketChannel> pUpstreamChannel, Step* pNextStep, ev_tstamp dTimeout)
    : Step(ACT_HTTP_STEP, pNextStep, dTimeout),
      m_pUpstreamChannel(pUpstreamChannel)
{
}

HttpStep::~HttpStep()
{
}

bool HttpStep::HttpPost(const std::string& strUrl, const std::string& strBody, const std::map<std::string, std::string>& mapHeaders)
{
    HttpMsg oHttpMsg;
    oHttpMsg.set_http_major(1);
    oHttpMsg.set_http_minor(1);
    oHttpMsg.set_type(HTTP_REQUEST);
    oHttpMsg.set_method(HTTP_POST);
    oHttpMsg.set_url(strUrl);
    HttpMsg::Header* pHeader = NULL;
    for (std::map<std::string, std::string>::const_iterator c_iter = mapHeaders.begin();
                    c_iter != mapHeaders.end(); ++c_iter)
    {
        pHeader = oHttpMsg.add_headers();
        pHeader->set_header_name(c_iter->first);
        pHeader->set_header_value(c_iter->second);
    }
    oHttpMsg.set_body(strBody);
    return(HttpRequest(oHttpMsg));
}

bool HttpStep::HttpGet(const std::string& strUrl)
{
    HttpMsg oHttpMsg;
    oHttpMsg.set_http_major(1);
    oHttpMsg.set_http_minor(1);
    oHttpMsg.set_type(HTTP_REQUEST);
    oHttpMsg.set_method(HTTP_GET);
    oHttpMsg.set_url(strUrl);
    return(HttpRequest(oHttpMsg));
}

bool HttpStep::HttpRequest(const HttpMsg& oHttpMsg)
{
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

        return(SendTo(strHost, iPort, strPath, oHttpMsg));
    }
    else
    {
        LOG4_ERROR("http_parser_parse_url \"%s\" error!", oHttpMsg.url().c_str());
        return(false);
    }
}


} /* namespace neb */
