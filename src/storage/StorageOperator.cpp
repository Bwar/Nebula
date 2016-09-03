/*******************************************************************************
 * Project:  Nebula
 * @file     StorageOperator.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#include "StorageOperator.hpp"

namespace neb
{

StorageOperator::StorageOperator(uint32 uiSectionFactor,
        const std::string& strTableName,
        Storage::DbOperate::E_QUERY_TYPE eQueryType,
        const std::string& strRedisKey, const std::string& strWriteCmd,
        const std::string& strReadCmd, uint32 uiModFactor)
    : RedisOperator(uiSectionFactor, strRedisKey, strWriteCmd, strReadCmd),
      DbOperator(uiSectionFactor, strTableName, eQueryType, uiModFactor),
      m_pMemRequest(NULL), m_uiSectionFactor(uiSectionFactor)
{
}

StorageOperator::~StorageOperator()
{
    if (m_pMemRequest != NULL)
    {
        delete m_pMemRequest;
        m_pMemRequest = NULL;
    }
}

Storage* StorageOperator::MakeMemOperate()
{
    if (m_pMemRequest == NULL)
    {
        m_pMemRequest = new Storage();
    }
    else
    {
        return (m_pMemRequest);
    }
    Storage::DbOperate* pDbOperate = GetDbOperate();
    Storage::RedisOperate* pRedisOperate = GetRedisOperate();
    m_pMemRequest->set_section_factor(m_uiSectionFactor);
    if (pRedisOperate != NULL)
    {
        m_pMemRequest->set_allocated_redis_operate(pRedisOperate);
        SetRedisOperateNull();
    }
    if (pDbOperate != NULL)
    {
        m_pMemRequest->set_allocated_db_operate(pDbOperate);
        SetDbOperateNull();
    }
    return (m_pMemRequest);
}

bool StorageOperator::AddField(const std::string& strFieldName,
        const std::string& strFieldValue, E_COL_TYPE eFieldType,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, strFieldValue, eFieldType,
                strColAs, bGroupBy, bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, strFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName, int32 iFieldValue,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, iFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, iFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName, uint32 uiFieldValue,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, uiFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, uiFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName, int64 llFieldValue,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, llFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, llFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName,
        uint64 ullFieldValue, int iFieldOper, const std::string& strColAs,
        bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, ullFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, ullFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName, float fFieldValue,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, fFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, fFieldValue);
    }
    return (true);
}

bool StorageOperator::AddField(const std::string& strFieldName, double dFieldValue,
        int iFieldOper, const std::string& strColAs, bool bGroupBy,
        bool bOrderBy, const std::string& strOrder)
{
    if (FIELD_OPERATOR_DB & iFieldOper)
    {
        DbOperator::AddDbField(strFieldName, dFieldValue, strColAs, bGroupBy,
                bOrderBy, strOrder);
    }
    if (FIELD_OPERATOR_REDIS & iFieldOper)
    {
        RedisOperator::AddRedisField(strFieldName, dFieldValue);
    }
    return (true);
}

} /* namespace neb */
