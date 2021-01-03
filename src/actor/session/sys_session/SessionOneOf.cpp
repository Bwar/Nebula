/*******************************************************************************
 * Project:  Nebula
 * @file     SessionOneOf.hpp
 * @brief    
 * @author   Bwar
 * @date:    2020-12-26
 * @note
 * Modify history:
 ******************************************************************************/
#include "SessionOneOf.hpp"
#include <algorithm>
#include "util/StringCoder.hpp"
#include "labor/Labor.hpp"
#include "ios/Dispatcher.hpp"

namespace neb
{

SessionOneOf::SessionOneOf(const std::string& strIdentify,  bool bWithSsl, bool bPipeline)
    : Timer(strIdentify, 300.0),
      m_bWithSsl(bWithSsl), m_bPipeline(bPipeline), m_uiAddressIndex(0)
{
    Split(strIdentify, ",", m_vecAddress);
}

SessionOneOf::~SessionOneOf()
{
}

bool SessionOneOf::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType)
{
    if (m_vecAddress.size() == 0)
    {
        return(false);
    }
    m_uiAddressIndex = (m_uiAddressIndex < m_vecAddress.size()) ? m_uiAddressIndex : 0;
    return(GetLabor(this)->GetDispatcher()->SendTo(m_vecAddress[m_uiAddressIndex++], eCodecType, false, true, iCmd, uiSeq, oMsgBody));
}

bool SessionOneOf::SendTo(const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq)
{
    if (m_vecAddress.size() == 0)
    {
        return(false);
    }
    m_uiAddressIndex = (m_uiAddressIndex < m_vecAddress.size()) ? m_uiAddressIndex : 0;
    bool bWithSsl = false;
    bool bPipeline = false;
    if (oHttpMsg.http_major() >= 2)
    {
        bPipeline = true;
    }
    std::string strSchema = oHttpMsg.url().substr(0, oHttpMsg.url().find_first_of(":"));
    std::transform(strSchema.begin(), strSchema.end(), strSchema.begin(), [](unsigned char c)->unsigned char {return std::tolower(c);});
    if (strSchema == std::string("https"))
    {
        bWithSsl = true;
    }
    return(GetLabor(this)->GetDispatcher()->SendTo(m_vecAddress[m_uiAddressIndex++], CODEC_HTTP, bWithSsl, bPipeline, oHttpMsg, uiStepSeq));
}

bool SessionOneOf::SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    if (m_vecAddress.size() == 0)
    {
        return(false);
    }
    m_uiAddressIndex = (m_uiAddressIndex < m_vecAddress.size()) ? m_uiAddressIndex : 0;
    return(GetLabor(this)->GetDispatcher()->SendTo(m_vecAddress[m_uiAddressIndex++], CODEC_RESP, bWithSsl, bPipeline, oRedisMsg, uiStepSeq));
}

bool SessionOneOf::SendTo(const std::string& strIdentify, const char* pRawData, uint32 uiRawDataSize, bool bWithSsl, bool bPipeline, uint32 uiStepSeq)
{
    if (m_vecAddress.size() == 0)
    {
        return(false);
    }
    m_uiAddressIndex = (m_uiAddressIndex < m_vecAddress.size()) ? m_uiAddressIndex : 0;
    return(GetLabor(this)->GetDispatcher()->SendTo(m_vecAddress[m_uiAddressIndex++], CODEC_RESP, bWithSsl, bPipeline, pRawData, uiRawDataSize, uiStepSeq));
}

} /* namespace neb */

