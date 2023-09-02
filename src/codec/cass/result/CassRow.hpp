/*******************************************************************************
 * Project:  Nebula
 * @file     CassRow.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_RESULT_CASSROW_HPP_
#define SRC_CODEC_CASSANDRA_RESULT_CASSROW_HPP_

#include <string>
#include <vector>
#include "Definition.hpp"
#include "type/Notations.hpp"

namespace neb
{

class CassResponse;

class CassRow
{
public:
    CassRow();
    CassRow(const CassRow&) = delete;
    CassRow(CassRow&& oRow);
    virtual ~CassRow();

    CassRow& operator=(const CassRow&) = delete;
    CassRow& operator=(CassRow&& oRow);
    const Bytes* operator[](uint16 unIndex) const;
    uint32 Size() const
    {
        return(m_vecColumnValue.size());
    }

private:
    friend class CassResponse;
    std::vector<Bytes> m_vecColumnValue;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_RESULT_CASSROW_HPP_ */

