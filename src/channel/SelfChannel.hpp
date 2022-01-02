/*******************************************************************************
 * Project:  Nebula
 * @file     SelfChannel.hpp
 * @brief    自我通信通道
 * @author   Bwar
 * @date:    2021-6-19
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SELFCHANNEL_HPP_
#define SRC_CHANNEL_SELFCHANNEL_HPP_

#include "SocketChannel.hpp"

namespace neb
{

class SelfChannel: public SocketChannel
{
public:
    SelfChannel();
    virtual ~SelfChannel();
    
    virtual int GetFd() const override
    {
        return(0);
    }

    virtual bool IsClient() const override
    {
        return(false);
    }

    virtual bool IsPipeline() const override
    {
        return(false);
    }

    virtual const std::string& GetIdentify() const override
    {
        return(strEmpty);
    }

    virtual const std::string& GetRemoteAddr() const override
    {
        return(strEmpty);
    }

    virtual const std::string& GetClientData() const override
    {
        return(strEmpty);
    }

    virtual E_CODEC_TYPE GetCodecType() const override
    {
        return(CODEC_DIRECT);
    }

    void SetStepSeq(uint32 uiStepSeq)
    {
        m_uiStepSeq = uiStepSeq;
    }

    uint32 GetStepSeq() const
    {
        return(m_uiStepSeq);
    }

    bool IsResponse() const
    {
        return(m_bIsResponse);
    }

    void SetResponse()
    {
        m_bIsResponse = true;
    }

private:
    bool m_bIsResponse = false;
    uint32 m_uiStepSeq = 0;
    std::string strEmpty;
};

} /* namespace neb */

#endif /* SRC_CHANNEL_SELFCHANNEL_HPP_ */

