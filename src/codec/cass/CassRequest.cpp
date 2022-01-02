/*******************************************************************************
 * Project:  Nebula
 * @file     CassRequest.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#include "CassRequest.hpp"
#include <iostream>

namespace neb
{

CassRequest::CassRequest()
    : CassMessage(CASS_MSG_REQUEST)
{
}

CassRequest::~CassRequest()
{
}

void CassRequest::SetQuery(const std::string& strQuery)
{
    SetOpcode(CASS_OP_QUERY);
    m_strQuery = strQuery;
}

void CassRequest::SetQuery(const std::string& strQuery, QueryParameter& stQueryParam)
{
    SetOpcode(CASS_OP_QUERY);
    m_strQuery = strQuery;
    m_stQueryParam = std::move(stQueryParam);
}

bool CassRequest::EncodeStarup(CBuffer* pBuff, const std::string& strCqlVersion, const std::string& strCompression) const
{
    std::unordered_map<std::string, std::string> mapOption;
    mapOption.insert(std::make_pair("CQL_VERSION", strCqlVersion));
    if (strCompression.size() > 0)
    {
        mapOption.insert(std::make_pair("COMPRESSION", strCompression));
    }
    return(Notations::EncodeStringMap(mapOption, pBuff));
}

bool CassRequest::EncodeAuthResponse(const std::string& strToken, CBuffer* pBuff) const
{
    Bytes stValue;
    stValue.Assign(strToken.data(), strToken.length());
    return(Notations::EncodeBytes(stValue, pBuff));
}

bool CassRequest::EncodeQuery(CBuffer* pBuff) const
{
    if (!Notations::EncodeLongString(m_strQuery, pBuff))
    {
        return(false);
    }
    Notations::EncodeShort(m_stQueryParam.unConsistency, pBuff);
    uint8 ucFlags = 0;
    if (m_stQueryParam.vecValue.size() > 0)
    {
        ucFlags |= QUERY_FLAG_VALUES;
        for (uint32 i = 0; i < m_stQueryParam.vecValue.size(); ++i)
        {
            if (m_stQueryParam.vecValue[i].first.size() > 0)
            {
                ucFlags |= QUERY_FLAG_WITH_NAMES_FOR_VALUE;
                break;
            }
        }
    }
    if (m_stQueryParam.bSkipMetadata)
    {
        ucFlags |= QUERY_FLAG_SKIP_METADATA;
    }
    if (m_stQueryParam.iPageSize > 0)
    {
        ucFlags |= QUERY_FLAG_PAGE_SIZE;
    }
    if (m_stQueryParam.stPagingState.iLength > 0)
    {
        ucFlags |= QUERY_FLAG_WITH_PAGING_STATE;
    }
    if (m_stQueryParam.unSerialConsistency > 0)
    {
        ucFlags |= QUERY_FLAG_WITH_SERIAL_CONSISTENCY;
    }
    if (m_stQueryParam.llTimestamp > 0)
    {
        ucFlags |= QUERY_FLAG_WITH_TIMESTAMP;
    }
    Notations::EncodeByte(ucFlags, pBuff);
    if (ucFlags & QUERY_FLAG_VALUES)
    {
        Notations::EncodeShort(m_stQueryParam.vecValue.size(), pBuff);
        if (ucFlags & QUERY_FLAG_WITH_NAMES_FOR_VALUE)
        {
            for (uint32 i = 0; i < m_stQueryParam.vecValue.size(); ++i)
            {
                if (!Notations::EncodeString(m_stQueryParam.vecValue[i].first, pBuff)
                        || !Notations::EncodeValue(m_stQueryParam.vecValue[i].second, pBuff))
                {
                    return(false);
                }
            }
        }
        else
        {
            for (uint32 i = 0; i < m_stQueryParam.vecValue.size(); ++i)
            {
                if (!Notations::EncodeValue(m_stQueryParam.vecValue[i].second, pBuff))
                {
                    return(false);
                }
            }
        }
    }
    if (ucFlags & QUERY_FLAG_PAGE_SIZE)
    {
        Notations::EncodeInt(m_stQueryParam.iPageSize, pBuff);
    }
    if (ucFlags & QUERY_FLAG_WITH_PAGING_STATE)
    {
        Notations::EncodeBytes(m_stQueryParam.stPagingState, pBuff);
    }
    if (ucFlags & QUERY_FLAG_WITH_SERIAL_CONSISTENCY)
    {
        Notations::EncodeShort(m_stQueryParam.unSerialConsistency, pBuff);
    }
    if (ucFlags & QUERY_FLAG_WITH_TIMESTAMP)
    {
        Notations::EncodeLong(m_stQueryParam.llTimestamp, pBuff);
    }
    return(true);
}

bool CassRequest::EncodePrepare(const std::string& strQuery, CBuffer* pBuff) const
{
    return(Notations::EncodeLongString(strQuery, pBuff));
}

} /* namespace neb */

