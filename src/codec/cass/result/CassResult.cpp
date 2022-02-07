/*******************************************************************************
 * Project:  Nebula
 * @file     CassResult.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#include "CassResult.hpp"

namespace neb
{

CassResult::CassResult()
{
}

CassResult::~CassResult()
{
}

CassResult::CassResult(CassResult&& oResult)
{
    m_iKind = oResult.m_iKind;
    m_vecMetaData = std::move(oResult.m_vecMetaData);
    m_vecRow = std::move(oResult.m_vecRow);
    m_oEmptyRow = std::move(oResult.m_oEmptyRow);
}

CassResult& CassResult::operator=(CassResult&& oResult)
{
    m_iKind = oResult.m_iKind;
    m_vecMetaData = std::move(oResult.m_vecMetaData);
    m_vecRow = std::move(oResult.m_vecRow);
    m_oEmptyRow = std::move(oResult.m_oEmptyRow);
    return(*this);
}

const CassRow& CassResult::operator[](uint16 unIndex) const
{
    if (unIndex < m_vecRow.size())
    {
        return(m_vecRow[unIndex]);
    }
    return(m_oEmptyRow);
}

bool CassResult::WithOptionValue(uint16 unOptionId)
{
    switch (unOptionId)
    {
        case CASS_RESULT_COL_CUSTOM:
        case CASS_RESULT_COL_LIST:
        case CASS_RESULT_COL_MAP:
        case CASS_RESULT_COL_SET:
        case CASS_RESULT_COL_UDT:
        case CASS_RESULT_COL_TUPLE:
            return(true);
        default:
            return(false);
    }
}

} /* namespace neb */

