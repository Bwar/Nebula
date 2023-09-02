/*******************************************************************************
 * Project:  Nebula
 * @file     CassResponse.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-27
 * @note
 * Modify history:
 ******************************************************************************/
#include "CassResponse.hpp"
#include "result/CassResult.hpp"

namespace neb
{

CassResponse::CassResponse()
    : CassMessage(CASS_MSG_RESPONSE)
{
}

CassResponse::~CassResponse()
{
}

CassResponse::CassResponse(CassResponse&& oMessage)
{
    m_bCassOk = oMessage.m_bCassOk;
    m_iErrCode = oMessage.m_iErrCode;
    m_strErrMsg = std::move(oMessage.m_strErrMsg);
    m_oResult = std::move(oMessage.m_oResult);
}

CassResponse& CassResponse::operator=(CassResponse&& oMessage)
{
    m_bCassOk = oMessage.m_bCassOk;
    m_iErrCode = oMessage.m_iErrCode;
    m_strErrMsg = std::move(oMessage.m_strErrMsg);
    m_oResult = std::move(oMessage.m_oResult);
    return(*this);
}

bool CassResponse::DecodeError(CBuffer* pBuff)
{
    m_bCassOk = false;
    return(Notations::DecodeInt(pBuff, m_iErrCode) && Notations::DecodeString(pBuff, m_strErrMsg));
}

bool CassResponse::DecodeReady(CBuffer* pBuff)
{
    return(true);
}

bool CassResponse::DecodeAuthenticate(CBuffer* pBuff, std::string& strAuthenticator)
{
    return(Notations::DecodeString(pBuff, strAuthenticator));
}

bool CassResponse::DecodeSupported(CBuffer* pBuff, std::unordered_map<std::string, std::vector<std::string>>& mapSupportedOptions)
{
    return(Notations::DecodeStringMultimap(pBuff, mapSupportedOptions));
}

bool CassResponse::DecodeResult(CBuffer* pBuff)
{
    if (!Notations::DecodeInt(pBuff, m_oResult.m_iKind))
    {
        return(false);
    }
    switch (m_oResult.m_iKind)
    {
        case CASS_RESULT_KIND_ROWS:
            return(DecodeRows(pBuff, m_oResult));
            break;
        case CASS_RESULT_KIND_VOID:
            break;
        case CASS_RESULT_KIND_SCHEMA_CHANGE:
            // TODO schema change
            break;
        case CASS_RESULT_KIND_SET_KEYSPACE:
            return(Notations::DecodeString(pBuff, m_oResult.m_strKeySpace));
            break;
        case CASS_RESULT_KIND_PREPARED:
            return(DecodePrepared(pBuff, m_oResult));
            break;
        default:
            return(false);
    }
    return(true);
}

bool CassResponse::DecodeRows(CBuffer* pBuff, CassResult& oResult)
{
    int32 iFlags = 0;
    int32 iColumnCount = 0;
    if (Notations::DecodeInt(pBuff, iFlags)
            && Notations::DecodeInt(pBuff, iColumnCount))
    {
        std::string strKeySpace;
        std::string strTableName;
        std::string strColumnName;
        uint16 unColumnType = 0;
        Value stValue;
        if (CASS_RESULT_FLAG_HAS_MORE_PAGES & iFlags)
        {
            if (!Notations::DecodeBytes(pBuff, oResult.m_stPagingState))
            {
                return(false);
            }
        }
        if (!(CASS_RESULT_FLAG_NO_METADATA & iFlags))
        {
            if (CASS_RESULT_FLAG_GLOBAL_TABLES_SPEC & iFlags)
            {
                if (!Notations::DecodeString(pBuff, strKeySpace)
                    || !Notations::DecodeString(pBuff, strTableName))
                {
                    return(false);
                }
                for (int i = 0; i < iColumnCount; ++i)
                {
                    if (!Notations::DecodeString(pBuff, strColumnName)
                            || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                    {
                        return(false);
                    }
                    CassResultMetaData stMetaData;
                    stMetaData.strColumnName = strColumnName;
                    stMetaData.strKeySpace = strKeySpace;
                    stMetaData.strTableName = strTableName;
                    stMetaData.unColumnType = unColumnType;
                    oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                }
            }
            else
            {
                for (int i = 0; i < iColumnCount; ++i)
                {
                    if (!Notations::DecodeString(pBuff, strKeySpace)
                            || !Notations::DecodeString(pBuff, strTableName)
                            || !Notations::DecodeString(pBuff, strColumnName)
                            || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                    {
                        return(false);
                    }
                    CassResultMetaData stMetaData;
                    stMetaData.strColumnName = strColumnName;
                    stMetaData.strKeySpace = strKeySpace;
                    stMetaData.strTableName = strTableName;
                    stMetaData.unColumnType = unColumnType;
                    oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                }
            }
        }

        int32 iRowCount = 0;
        if (Notations::DecodeInt(pBuff, iRowCount))
        {
            for (int j = 0; j < iRowCount; ++j)
            {
                CassRow oRow;
                for (int k = 0; k < iColumnCount; ++k)
                {
                    Bytes stByte;
                    if (!Notations::DecodeBytes(pBuff, stByte))
                    {
                        return(false);
                    }
                    oRow.m_vecColumnValue.emplace_back(std::move(stByte));
                }
                oResult.m_vecRow.emplace_back(std::move(oRow));
            }
            return(true);
        }
    }
    return(false);
}

bool CassResponse::DecodePrepared(CBuffer* pBuff, CassResult& oResult)
{
    if (Notations::DecodeShortBytes(pBuff, oResult.m_stQueryId))
    {
        int32 iFlags = 0;
        int32 iColumnCount = 0;
        int32 iPkCount = 0;
        if (Notations::DecodeInt(pBuff, iFlags)
                && Notations::DecodeInt(pBuff, iColumnCount)
                && Notations::DecodeInt(pBuff, iPkCount))
        {
            uint16 unColumnType = 0;
            uint16 unPkIndex = 0;
            std::string strKeySpace;
            std::string strTableName;
            std::string strColumnName;
            Value stValue;
            for (int i = 0; i < iPkCount; ++i)
            {
                if (!Notations::DecodeShort(pBuff, unPkIndex))
                {
                    return(false);
                }
                oResult.m_vecPkIndex.push_back(unPkIndex);
            }
            if (CASS_RESULT_FLAG_GLOBAL_TABLES_SPEC & iFlags)
            {
                if (!Notations::DecodeString(pBuff, strKeySpace)
                    || !Notations::DecodeString(pBuff, strTableName))
                {
                    return(false);
                }
                for (int i = 0; i < iColumnCount; ++i)
                {
                    if (!Notations::DecodeString(pBuff, strColumnName)
                            || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                    {
                        return(false);
                    }
                    CassResultMetaData stMetaData;
                    stMetaData.strColumnName = strColumnName;
                    stMetaData.strKeySpace = strKeySpace;
                    stMetaData.strTableName = strTableName;
                    stMetaData.unColumnType = unColumnType;
                    oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                }
            }
            else
            {
                for (int i = 0; i < iColumnCount; ++i)
                {
                    if (!Notations::DecodeString(pBuff, strKeySpace)
                            || !Notations::DecodeString(pBuff, strTableName)
                            || !Notations::DecodeString(pBuff, strColumnName)
                            || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                    {
                        return(false);
                    }
                    CassResultMetaData stMetaData;
                    stMetaData.strColumnName = strColumnName;
                    stMetaData.strKeySpace = strKeySpace;
                    stMetaData.strTableName = strTableName;
                    stMetaData.unColumnType = unColumnType;
                    oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                }
            }

            // TODO result-metadata
            if (!(CASS_RESULT_FLAG_NO_METADATA & iFlags))
            {
                if (CASS_RESULT_FLAG_GLOBAL_TABLES_SPEC & iFlags)
                {
                    if (!Notations::DecodeString(pBuff, strKeySpace)
                        || !Notations::DecodeString(pBuff, strTableName))
                    {
                        return(false);
                    }
                    for (int i = 0; i < iColumnCount; ++i)
                    {
                        if (!Notations::DecodeString(pBuff, strColumnName)
                                || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                        {
                            return(false);
                        }
                        CassResultMetaData stMetaData;
                        stMetaData.strColumnName = strColumnName;
                        stMetaData.strKeySpace = strKeySpace;
                        stMetaData.strTableName = strTableName;
                        stMetaData.unColumnType = unColumnType;
                        oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                    }
                }
                else
                {
                    for (int i = 0; i < iColumnCount; ++i)
                    {
                        if (!Notations::DecodeString(pBuff, strKeySpace)
                                || !Notations::DecodeString(pBuff, strTableName)
                                || !Notations::DecodeString(pBuff, strColumnName)
                                || !Notations::DecodeOption(pBuff, CassResult::WithOptionValue, unColumnType, stValue))
                        {
                            return(false);
                        }
                        CassResultMetaData stMetaData;
                        stMetaData.strColumnName = strColumnName;
                        stMetaData.strKeySpace = strKeySpace;
                        stMetaData.strTableName = strTableName;
                        stMetaData.unColumnType = unColumnType;
                        oResult.m_vecMetaData.emplace_back(std::move(stMetaData));
                    }
                }
            }
            return(true);
        }
    }
    return(false);
}

} /* namespace neb */

