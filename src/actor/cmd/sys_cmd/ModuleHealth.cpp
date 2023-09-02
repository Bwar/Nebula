/*******************************************************************************
 * Project:  Nebula
 * @file     ModuleHealth.cpp
 * @brief
 * @author   Bwar
 * @date:    2020-2-16
 * @note
 * Modify history:
 ******************************************************************************/

#include "ModuleHealth.hpp"

namespace neb
{

ModuleHealth::ModuleHealth(const std::string& strModulePath)
    : Module(strModulePath)
{
}

ModuleHealth::~ModuleHealth()
{
}

bool ModuleHealth::AnyMessage(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg)
{
    HttpMsg oOutHttpMsg;
    oOutHttpMsg.set_type(HTTP_RESPONSE);
    oOutHttpMsg.set_status_code(200);
    oOutHttpMsg.set_http_major(oHttpMsg.http_major());
    oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
    SendTo(pChannel, oOutHttpMsg);
    return(true);
}

}
