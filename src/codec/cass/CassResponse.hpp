/*******************************************************************************
 * Project:  Nebula
 * @file     CassResponse.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-27
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_CASSRESPONSE_HPP_
#define SRC_CODEC_CASSANDRA_CASSRESPONSE_HPP_

#include "CassMessage.hpp"
#include "result/CassResult.hpp"

namespace neb
{

class CodecCass;

class CassResponse: public CassMessage
{
public:
    CassResponse();
    CassResponse(const CassResponse&) = delete;
    CassResponse(CassResponse&&);
    virtual ~CassResponse();
    CassResponse& operator=(const CassResponse&) = delete;
    CassResponse& operator=(CassResponse&&);

    bool IsSuccess() const
    {
        return(m_bCassOk);
    }

    const CassResult& GetResult() const
    {
        return(m_oResult);
    }

    int32 GetErrCode() const
    {
        return(m_iErrCode);
    }

    const std::string& GetErrMsg() const
    {
        return(m_strErrMsg);
    }

protected:
    bool DecodeError(CBuffer* pBuffg);
    bool DecodeReady(CBuffer* pBuff);
    bool DecodeAuthenticate(CBuffer* pBuff, std::string& strAuthenticator);
    bool DecodeSupported(CBuffer* pBuff, std::unordered_map<std::string, std::vector<std::string>>& mapSupportedOptions);
    bool DecodeResult(CBuffer* pBuff);
    // TODO bool DecodeEvent();
    // TODO bool DecodeAuthChallenge();
    // TODO bool DecodeAuthSuccess();

protected:
    bool DecodeRows(CBuffer* pBuff, CassResult& oResult);
    bool DecodePrepared(CBuffer* pBuff, CassResult& oResult);

private:
    friend class CodecCass;
    bool m_bCassOk = true;
    int32 m_iErrCode = 0;
    std::string m_strErrMsg;
    CassResult m_oResult;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_CASSRESPONSE_HPP_ */

