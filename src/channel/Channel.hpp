/*******************************************************************************
* Project:  Nebula
* @file     Channel.hpp
* @brief 
* @author   bwar
* @date:    Mar 25, 2018
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_CHANNEL_CHANNEL_HPP_
#define SRC_CHANNEL_CHANNEL_HPP_

#include "Definition.hpp"

namespace neb
{

struct ChannelOption
{
    bool bPipeline = false;
    bool bWithSsl = false;
    int iSocketType = SOCKET_STREAM;
    uint32 uiMaxSendBuffSize = gc_iMaxBuffLen;
    ev_tstamp dKeepAlive = 7.0;
    std::string strAuth;
    std::string strPassword;

    ChannelOption(){}
    ChannelOption(const ChannelOption& stOption)
    {
        bPipeline = stOption.bPipeline;
        bWithSsl = stOption.bWithSsl;
        iSocketType = stOption.iSocketType;
        dKeepAlive = stOption.dKeepAlive;
        strAuth = stOption.strAuth;
        strPassword = stOption.strPassword;
    }
    ChannelOption(ChannelOption&& stOption)
    {
        bPipeline = stOption.bPipeline;
        bWithSsl = stOption.bWithSsl;
        iSocketType = stOption.iSocketType;
        dKeepAlive = stOption.dKeepAlive;
        strAuth = std::move(stOption.strAuth);
        strPassword = std::move(stOption.strPassword);
    }
    ChannelOption& operator=(const ChannelOption& stOption)
    {
        bPipeline = stOption.bPipeline;
        bWithSsl = stOption.bWithSsl;
        iSocketType = stOption.iSocketType;
        dKeepAlive = stOption.dKeepAlive;
        strAuth = stOption.strAuth;
        strPassword = stOption.strPassword;
        return(*this);
    }
    ChannelOption& operator=(ChannelOption&& stOption)
    {
        bPipeline = stOption.bPipeline;
        bWithSsl = stOption.bWithSsl;
        iSocketType = stOption.iSocketType;
        dKeepAlive = stOption.dKeepAlive;
        strAuth = std::move(stOption.strAuth);
        strPassword = std::move(stOption.strPassword);
        return(*this);
    }
};

class Channel
{
public:
    Channel(){}
    virtual ~Channel(){}
};

}


#endif /* SRC_CHANNEL_CHANNEL_HPP_ */
