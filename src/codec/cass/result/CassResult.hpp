/*******************************************************************************
 * Project:  Nebula
 * @file     CassResult.hpp
 * @brief
 * @author   Bwar
 * @date:    2021-11-28
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CASSANDRA_RESULT_CASSRESULT_HPP_
#define SRC_CODEC_CASSANDRA_RESULT_CASSRESULT_HPP_

#include <string>
#include <vector>
#include "Definition.hpp"
#include "CassRow.hpp"

namespace neb
{

enum E_CASS_RESULT_KIND
{
    CASS_RESULT_KIND_VOID               = 0x0001,   ///< Void: for results carrying no information.
    CASS_RESULT_KIND_ROWS               = 0x0002,   ///< Rows: for results to select queries, returning a set of rows.
    CASS_RESULT_KIND_SET_KEYSPACE       = 0x0003,   ///< Set_keyspace: the result to a `use` query.
    CASS_RESULT_KIND_PREPARED           = 0x0004,   ///< Prepared: result to a PREPARE message.
    CASS_RESULT_KIND_SCHEMA_CHANGE      = 0x0005,   ///< Schema_change: the result to a schema altering query.
};

enum E_CASS_RESULT_FLAG
{
    CASS_RESULT_FLAG_GLOBAL_TABLES_SPEC  = 0x0001,   ///< if set, only one table spec (keyspace and table name) is provided as <global_table_spec>. If not set, <global_table_spec> is not present.
    CASS_RESULT_FLAG_HAS_MORE_PAGES      = 0x0002,   ///< indicates whether this is not the last page of results and more should be retrieved.
    CASS_RESULT_FLAG_NO_METADATA         = 0x0004,   ///< No_metadata
};

enum E_CASS_RESULT_COL_TYPE
{
    CASS_RESULT_COL_CUSTOM              = 0x0000,   ///< the value is a [string].
    CASS_RESULT_COL_ASCII               = 0x0001,
    CASS_RESULT_COL_BIGINT              = 0x0002,
    CASS_RESULT_COL_BLOB                = 0x0003,
    CASS_RESULT_COL_BOOLEAN             = 0x0004,
    CASS_RESULT_COL_COUNTER             = 0x0005,
    CASS_RESULT_COL_DECIMAL             = 0x0006,
    CASS_RESULT_COL_DOUBLE              = 0x0007,
    CASS_RESULT_COL_FLOAT               = 0x0008,
    CASS_RESULT_COL_INT                 = 0x0009,
    CASS_RESULT_COL_TIMESTAMP           = 0x000B,
    CASS_RESULT_COL_UUID                = 0x000C,
    CASS_RESULT_COL_VARCHAR             = 0x000D,
    CASS_RESULT_COL_VARINT              = 0x000E,
    CASS_RESULT_COL_TIMEUUID            = 0x000F,
    CASS_RESULT_COL_INET                = 0x0010,
    CASS_RESULT_COL_DATE                = 0x0011,
    CASS_RESULT_COL_TIME                = 0x0012,
    CASS_RESULT_COL_SMALLINT            = 0x0013,
    CASS_RESULT_COL_TINYINT             = 0x0014,
    CASS_RESULT_COL_LIST                = 0x0020,   ///< the value is an [option], representing the type of the elements of the list.
    CASS_RESULT_COL_MAP                 = 0x0021,   ///< the value is two [option], representing the types of the keys and values of the map
    CASS_RESULT_COL_SET                 = 0x0022,   ///< the value is an [option], representing the type of the elements of the set
    CASS_RESULT_COL_UDT                 = 0x0030,   ///< the value is <ks><udt_name><n><name_1><type_1>...<name_n><type_n>
    CASS_RESULT_COL_TUPLE               = 0x0031,   ///< the value is <n><type_1>...<type_n> where <n> is a [short] representing the number of values in the type, and <type_i> are [option] representing the type of the i_th component of the tuple
};

/**
 * @brief <metadata> is composed of: <flags><columns_count>[<paging_state>][<global_table_spec>?<col_spec_1>...<col_spec_n>]
 * @note - <flags> is an [int]. The bits of <flags> provides information on the
 *         formatting of the remaining information. A flag is set if the bit
 *         corresponding to its `mask` is set. Supported flags are, given their CassResult::FLAG
 *       - <columns_count> is an [int] representing the number of columns selected
 *         by the query that produced this result. It defines the number of <col_spec_i>
 *         elements in and the number of elements for each row in <rows_content>.
 *       - <global_table_spec> is present if the Global_tables_spec is set in
 *         <flags>. It is composed of two [string] representing the
 *         (unique) keyspace name and table name the columns belong to.
 *       - <col_spec_i> specifies the columns returned in the query. There are
 *         <column_count> such column specifications that are composed of:
 */
struct CassResultMetaData
{
    uint16 unColumnType     = 0;
    std::string strColumnName;
    std::string strTableName;
    std::string strKeySpace;

    CassResultMetaData(){}
    ~CassResultMetaData()
    {
    }
    CassResultMetaData(const CassResultMetaData&) = delete;
    CassResultMetaData(CassResultMetaData&& stMeta)
    {
        unColumnType = stMeta.unColumnType;
        strColumnName = std::move(stMeta.strColumnName);
        strTableName = std::move(stMeta.strTableName);
        strKeySpace = std::move(stMeta.strKeySpace);
    }
    CassResultMetaData& operator=(const CassResultMetaData& stMeta)
    {
        unColumnType = stMeta.unColumnType;
        strColumnName = std::move(stMeta.strColumnName);
        strTableName = std::move(stMeta.strTableName);
        strKeySpace = std::move(stMeta.strKeySpace);
        return(*this);
    }
    CassResultMetaData& operator=(CassResultMetaData&& stMeta)
    {
        unColumnType = stMeta.unColumnType;
        strColumnName = std::move(stMeta.strColumnName);
        strTableName = std::move(stMeta.strTableName);
        strKeySpace = std::move(stMeta.strKeySpace);
        return(*this);
    }
};

class CassResponse;

class CassResult
{
public:
    CassResult();
    CassResult(const CassResult&) = delete;
    CassResult(CassResult&& oResult);
    virtual ~CassResult();

    CassResult& operator=(const CassResult&) = delete;
    CassResult& operator=(CassResult&& oResult);
    const CassRow& operator[](uint16 unIndex) const;

    uint32 Size() const
    {
        return(m_vecRow.size());
    }

    const Bytes& GetQueryId() const
    {
        return(m_stQueryId);
    }

    const std::vector<uint16>& GetPkIndex() const
    {
        return(m_vecPkIndex);
    }

    static bool WithOptionValue(uint16 unOptionId);

private:
    friend class CassResponse;

    int32 m_iKind = 0;
    std::string m_strKeySpace;      // for SET_KEYSPACE or SCHEMA_CHANGE
    Bytes m_stQueryId;              // for PREPARED
    Bytes m_stPagingState;
    std::vector<uint16> m_vecPkIndex;   // for PREPARED
    std::vector<CassResultMetaData> m_vecMetaData;
    std::vector<CassRow> m_vecRow;
    CassRow m_oEmptyRow;
};

} /* namespace neb */

#endif /* SRC_CODEC_CASSANDRA_RESULT_CASSRESULT_HPP_ */

