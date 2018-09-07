/*******************************************************************************
 * Project:  Nebula
 * @file     DbOperator.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#include "DbOperator.hpp"

namespace neb
{

DbOperator::DbOperator(
        uint32 uiSectionFactor,
        const std::string& strTableName,
        Mydis::DbOperate::E_QUERY_TYPE eQueryType,
        uint32 uiModFactor)
    : m_pDbMemRequest(nullptr), m_pDbOperate(nullptr), m_uiSectionFactor(uiSectionFactor)
{
    m_pDbOperate = new Mydis::DbOperate();
    m_pDbOperate->set_table_name(strTableName);
    m_pDbOperate->set_query_type(eQueryType);
    if (uiModFactor > 0)
    {
        m_pDbOperate->set_mod_factor(uiModFactor);
    }
}

DbOperator::~DbOperator()
{
    if (m_pDbMemRequest != nullptr)
    {
        delete m_pDbMemRequest;
        m_pDbMemRequest = nullptr;
    }
    else
    {
        if (m_pDbOperate != nullptr)
        {
            delete m_pDbOperate;
            m_pDbOperate = nullptr;
        }
    }
}

Mydis* DbOperator::MakeMemOperate()
{
    if (m_pDbMemRequest == nullptr)
    {
        m_pDbMemRequest = new Mydis();
    }
    else
    {
        return(m_pDbMemRequest);
    }
    m_pDbMemRequest->set_section_factor(m_uiSectionFactor);
    m_pDbMemRequest->set_allocated_db_operate(m_pDbOperate);
    return(m_pDbMemRequest);
}

bool DbOperator::AddDbField(const std::string& strFieldName, const std::string& strFieldValue,
                E_COL_TYPE eFieldType, const std::string& strColAs,
                bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    Field* pField = m_pDbOperate->add_fields();
    pField->set_col_name(strFieldName);
    pField->set_col_type(eFieldType);
    pField->set_col_value(strFieldValue);
    if (strColAs.size() > 0)
    {
        pField->set_col_as(strColAs);
    }
    if (bGroupBy)
    {
        m_pDbOperate->add_groupby_col(strFieldName);
    }
    if (bOrderBy)
    {
        Mydis::DbOperate::OrderBy *pOrderBy = m_pDbOperate->add_orderby_col();
        pOrderBy->set_col_name(strFieldName);
        if (std::string("DESC") == strOrder || std::string("desc") == strOrder)
        {
            pOrderBy->set_relation(Mydis::DbOperate::OrderBy::DESC);
        }
        else
        {
            pOrderBy->set_relation(Mydis::DbOperate::OrderBy::ASC);
        }
    }
    return(true);
}

bool DbOperator::AddDbField(const std::string& strFieldName, int32 iFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, INT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, INT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, int64 llFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, BIGINT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, BIGINT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, float fFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", fFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, FLOAT, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddDbField(const std::string& strFieldName, double dFieldValue,
                const std::string& strColAs, bool bGroupBy, bool bOrderBy, const std::string& strOrder)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddDbField(strFieldName, (std::string)szFieldValue, DOUBLE, strColAs, bGroupBy, bOrderBy, strOrder));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::string& strFieldValue,
                E_COL_TYPE eFieldType, const std::string& strRightFieldName)
{
    Mydis::DbOperate::ConditionGroup* pConditionGroup;
    Mydis::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(Mydis::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(eFieldType);
    if (strRightFieldName.length() > 0)
    {
        pCondition->set_col_name_right(strRightFieldName);
    }
    else
    {
        pCondition->add_col_values(strFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int32 iFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, INT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, INT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int64 llFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, float fFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%f", fFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, double dFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[64] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddCondition(eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::vector<uint32>& vecFieldValues)
{
    char szFieldValue[64] = {0};
    Mydis::DbOperate::ConditionGroup* pConditionGroup;
    Mydis::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(Mydis::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(INT);
    for (std::vector<uint32>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        snprintf(szFieldValue, sizeof(szFieldValue), "%u", *c_iter);
        pCondition->add_col_values(szFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::vector<uint64>& vecFieldValues)
{
    char szFieldValue[64] = {0};
    Mydis::DbOperate::ConditionGroup* pConditionGroup;
    Mydis::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(Mydis::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(BIGINT);
    for (std::vector<uint64>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        snprintf(szFieldValue, sizeof(szFieldValue), "%llu", *c_iter);
        pCondition->add_col_values(szFieldValue);
    }
    return(true);
}

bool DbOperator::AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::vector<std::string>& vecFieldValues)
{
    Mydis::DbOperate::ConditionGroup* pConditionGroup;
    Mydis::DbOperate::Condition* pCondition;
    if (m_pDbOperate->conditions_size() > 0)
    {
        int iConditionIdx = m_pDbOperate->conditions_size() - 1;
        pConditionGroup = m_pDbOperate->mutable_conditions(iConditionIdx);
        pCondition = pConditionGroup->add_condition();
    }
    else
    {
        pConditionGroup = m_pDbOperate->add_conditions();
        pCondition = pConditionGroup->add_condition();
    }
    pConditionGroup->set_relation(Mydis::DbOperate::ConditionGroup::AND);
    pCondition->set_relation(eRelation);
    pCondition->set_col_name(strFieldName);
    pCondition->set_col_type(STRING);
    for (std::vector<std::string>::const_iterator c_iter = vecFieldValues.begin();
                    c_iter != vecFieldValues.end(); ++c_iter)
    {
        pCondition->add_col_values(*c_iter);
    }
    return(true);
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, const std::string& strFieldValue,
                E_COL_TYPE eFieldType, const std::string& strRightFieldName)
{
    if (iGroupIdx >= m_pDbOperate->conditions_size())
    {
        Mydis::DbOperate::ConditionGroup* pConditionGroup = m_pDbOperate->add_conditions();
        pConditionGroup->set_relation(eGroupRelation);
        Mydis::DbOperate::Condition* pCondition = pConditionGroup->add_condition();
        pCondition->set_relation(eRelation);
        pCondition->set_col_name(strFieldName);
        pCondition->set_col_type(eFieldType);
        if (strRightFieldName.length() > 0)
        {
            pCondition->set_col_name_right(strRightFieldName);
        }
        else
        {
            pCondition->add_col_values(strFieldValue);
        }
    }
    else
    {
        Mydis::DbOperate::Condition* pCondition = m_pDbOperate->mutable_conditions(iGroupIdx)->add_condition();
        pCondition->set_relation(eRelation);
        pCondition->set_col_name(strFieldName);
        pCondition->set_col_type(eFieldType);
        if (strRightFieldName.length() > 0)
        {
            pCondition->set_col_name_right(strRightFieldName);
        }
        else
        {
            pCondition->add_col_values(strFieldValue);
        }
    }
    return(true);
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int32 iFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%d", iFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, INT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint32 uiFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%u", uiFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, INT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, int64 llFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lld", llFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, uint64 ullFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%llu", ullFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, BIGINT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, float fFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[40] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%f", fFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, FLOAT, strRightFieldName));
}

bool DbOperator::AddCondition(int iGroupIdx,
                Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                Mydis::DbOperate::Condition::E_RELATION eRelation,
                const std::string& strFieldName, double dFieldValue,
                const std::string& strRightFieldName)
{
    char szFieldValue[60] = {0};
    snprintf(szFieldValue, sizeof(szFieldValue), "%lf", dFieldValue);
    return(AddCondition(iGroupIdx, eGroupRelation, eRelation, strFieldName, szFieldValue, DOUBLE, strRightFieldName));
}

void DbOperator::SetConditionGroupRelation(Mydis::DbOperate::ConditionGroup::E_RELATION eRelation)
{
    m_pDbOperate->set_group_relation(eRelation);
}

void DbOperator::AddLimit(unsigned int uiLimitFrom, unsigned int uiLimit)
{
    m_pDbOperate->set_limit(uiLimit);
    if (uiLimitFrom > 0)
    {
        m_pDbOperate->set_limit_from(uiLimitFrom);
    }
}

void DbOperator::AddLimit(unsigned int uiLimit)
{
    m_pDbOperate->set_limit(uiLimit);
}

} /* namespace neb */
