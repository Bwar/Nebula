/*******************************************************************************
 * Project:  Nebula
 * @file     CassRequest.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_CASSREQUEST_HPP_
#define SRC_CODEC_CASSANDRA_CASSREQUEST_HPP_

#include "CassMessage.hpp"

namespace neb
{

class CodecCass;

class CassRequest: public CassMessage
{
public:
    enum QUERY_FLAG
    {
        QUERY_FLAG_VALUES                   = 0x01,
        QUERY_FLAG_SKIP_METADATA            = 0x02,
        QUERY_FLAG_PAGE_SIZE                = 0x04,
        QUERY_FLAG_WITH_PAGING_STATE        = 0x08,
        QUERY_FLAG_WITH_SERIAL_CONSISTENCY  = 0x10,
        QUERY_FLAG_WITH_TIMESTAMP           = 0x20,
        QUERY_FLAG_WITH_NAMES_FOR_VALUE     = 0x40,
    };

    struct QueryParameter
    {
        uint16 unConsistency            = 0;
        uint16 unSerialConsistency      = 0;
        bool bSkipMetadata              = false;
        int32 iPageSize                 = 0;
        Bytes stPagingState;
        int64 llTimestamp               = 0;
        std::vector<std::pair<std::string, Value>> vecValue;

        QueryParameter(){}
        QueryParameter(const QueryParameter&) = delete;
        QueryParameter(QueryParameter&& stParameter)
        {
            unConsistency = stParameter.unConsistency;
            unSerialConsistency = stParameter.unSerialConsistency;
            bSkipMetadata = stParameter.bSkipMetadata;
            iPageSize = stParameter.iPageSize;
            stPagingState = std::move(stParameter.stPagingState);
            llTimestamp = stParameter.llTimestamp;
            vecValue = std::move(stParameter.vecValue);
        }
        QueryParameter& operator=(const QueryParameter&) = delete;
        QueryParameter& operator=(QueryParameter&& stParameter)
        {
            unConsistency = stParameter.unConsistency;
            unSerialConsistency = stParameter.unSerialConsistency;
            bSkipMetadata = stParameter.bSkipMetadata;
            iPageSize = stParameter.iPageSize;
            stPagingState = std::move(stParameter.stPagingState);
            llTimestamp = stParameter.llTimestamp;
            vecValue = std::move(stParameter.vecValue);
            return(*this);
        }
    };

public:
    CassRequest();
    CassRequest(const CassRequest&) = delete;
    virtual ~CassRequest();

    CassRequest& operator=(const CassRequest&) = delete;

    void SetQuery(const std::string& strQuery);
    void SetQuery(const std::string& strQuery, QueryParameter& stQueryParam);

private:
    bool EncodeStarup(CBuffer* pBuff, const std::string& strCqlVersion = "3.0.0", const std::string& strCompression = "") const;
    bool EncodeAuthResponse(const std::string& strToken, CBuffer* pBuff) const;
    bool EncodeQuery(CBuffer* pBuff) const;  // TODO: query_parameters
    bool EncodePrepare(const std::string& strQuery, CBuffer* pBuff) const;
    // TODO
    // bool EncodeExcute()
    // bool EncodeBatch()
    // bool EncodeRegister()

private:
    friend class CodecCass;
    std::string m_strQuery;
    std::string m_strTracingId;
    std::vector<std::string> m_vecWarnings;
    std::unordered_map<std::string, Bytes> m_mapCustomPayload;
    QueryParameter m_stQueryParam;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_CASSREQUEST_HPP_ */

