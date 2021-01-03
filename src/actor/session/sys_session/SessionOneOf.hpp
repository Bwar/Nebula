/*******************************************************************************
 * Project:  Nebula
 * @file     SessionOneOf.hpp
 * @brief    
 * @author   Bwar
 * @date:    2020-12-26
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_SESSION_SYS_SESSION_SESSIONONEOF_H_
#define SRC_ACTOR_SESSION_SYS_SESSION_SESSIONONEOF_H_

#include "actor/session/Timer.hpp"
#include "actor/ActorSys.hpp"

namespace neb
{

class SessionOneOf: public Timer,
    DynamicCreator<SessionOneOf, const std::string&, bool, bool>,
    public ActorSys
{
public:
    SessionOneOf(const std::string& strIdentify,  bool bWithSsl, bool bPipeline);
    virtual ~SessionOneOf();

    virtual E_CMD_STATUS Timeout()
    {
        return(CMD_STATUS_COMPLETED);
    }

    virtual bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, E_CODEC_TYPE eCodecType = CODEC_NEBULA);
    virtual bool SendTo(const std::string& strHost, int iPort, const HttpMsg& oHttpMsg, uint32 uiStepSeq = 0);
    virtual bool SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg, bool bWithSsl = false, bool bPipeline = true, uint32 uiStepSeq = 0);
    virtual bool SendTo(const std::string& strIdentify, const char* pRawData, uint32 uiRawDataSize, bool bWithSsl = false, bool bPipeline = false, uint32 uiStepSeq = 0);

private:
    bool m_bWithSsl;                        ///< 是否支持SSL
    bool m_bPipeline;                       ///< 是否支持pipeline
    //std::string m_strIdentify;      ///< 地址标识，由m_vecAddress合并而成，形如 192.168.47.101:6379,198.168.47.102:6379,192.168.47.103:6379  GetSessionId()
    uint32 m_uiAddressIndex;
    std::vector<std::string> m_vecAddress;  ///< 集群地址
};

} /* namespace neb */

#endif /* SRC_ACTOR_SESSION_SYS_SESSION_SESSIONONEOF_H_ */

