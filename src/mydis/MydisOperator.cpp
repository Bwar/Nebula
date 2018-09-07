/*******************************************************************************
 * Project:  Nebula
 * @file     MydisOperator.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#include "MydisOperator.hpp"

namespace neb
{

MydisOperator::MydisOperator(uint32 uiSectionFactor,
        const std::string& strTableName,
        Mydis::DbOperate::E_QUERY_TYPE eQueryType,
        const std::string& strRedisKey, const std::string& strWriteCmd,
        const std::string& strReadCmd, uint32 uiModFactor)
    : RedisOperator(uiSectionFactor, strRedisKey, strWriteCmd, strReadCmd),
      DbOperator(uiSectionFactor, strTableName, eQueryType, uiModFactor),
      m_pMemRequest(nullptr), m_uiSectionFactor(uiSectionFactor)
{
}

MydisOperator::~MydisOperator()
{
    if (m_pMemRequest != nullptr)
    {
        delete m_pMemRequest;
        m_pMemRequest = nullptr;
    }
}

Mydis* MydisOperator::MakeMemOperate()
{
    if (m_pMemRequest == nullptr)
    {
        m_pMemRequest = new Mydis();
    }
    else
    {
        return (m_pMemRequest);
    }
    Mydis::DbOperate* pDbOperate = GetDbOperate();
    Mydis::RedisOperate* pRedisOperate = GetRedisOperate();
    m_pMemRequest->set_section_factor(m_uiSectionFactor);
    if (pRedisOperate != nullptr)
    {
        m_pMemRequest->set_allocated_redis_operate(pRedisOperate);
        SetRedisOperateNull();
    }
    if (pDbOperate != nullptr)
    {
        m_pMemRequest->set_allocated_db_operate(pDbOperate);
        SetDbOperateNull();
    }
    return (m_pMemRequest);
}

bool MydisOperator::AddField(const std::string& strFieldName,
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

bool MydisOperator::AddField(const std::string& strFieldName, int32 iFieldValue,
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

bool MydisOperator::AddField(const std::string& strFieldName, uint32 uiFieldValue,
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

bool MydisOperator::AddField(const std::string& strFieldName, int64 llFieldValue,
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

bool MydisOperator::AddField(const std::string& strFieldName,
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

bool MydisOperator::AddField(const std::string& strFieldName, float fFieldValue,
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

bool MydisOperator::AddField(const std::string& strFieldName, double dFieldValue,
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
