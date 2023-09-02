/*******************************************************************************
 * Project:  Nebula
 * @file     CassRow.cpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#include "CassRow.hpp"

namespace neb
{

CassRow::CassRow()
{
}

CassRow::~CassRow()
{
}

CassRow::CassRow(CassRow&& oRow)
{
    m_vecColumnValue = std::move(oRow.m_vecColumnValue);
}

CassRow& CassRow::operator=(CassRow&& oRow)
{
    m_vecColumnValue = std::move(oRow.m_vecColumnValue);
    return(*this);
}

const Bytes* CassRow::operator[](uint16 unIndex) const
{
    if (unIndex >= m_vecColumnValue.size())
    {
        return(nullptr);
    }
    return(&m_vecColumnValue[unIndex]);
}

} /* namespace neb */

