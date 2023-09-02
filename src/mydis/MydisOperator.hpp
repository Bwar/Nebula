/*******************************************************************************
 * Project:  Nebula
 * @file     MydisOperator.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STORAGE_STORAGEOPERATOR_HPP_
#define SRC_STORAGE_STORAGEOPERATOR_HPP_

#include "DbOperator.hpp"
#include "RedisOperator.hpp"

namespace neb
{

enum E_MEM_FIELD_OPERATOR
{
    FIELD_OPERATOR_DB = 0x0001,    ///< 对数据库有效
    FIELD_OPERATOR_REDIS = 0x0002,    ///< 对Redis有效
};

/**
 * @brief 存储请求协议生成器
 * @note 存储请求协议生成，用于同时需要请求Redis和数据库的场景。
 */
class MydisOperator: public RedisOperator, public DbOperator
{
public:
    MydisOperator(uint32 uiSectionFactor, const std::string& strTableName,
            Mydis::DbOperate::E_QUERY_TYPE eQueryType,
            const std::string& strRedisKey, const std::string& strWriteCmd,
            const std::string& strReadCmd = "", uint32 uiModFactor = 0);
    virtual ~MydisOperator();

    virtual Mydis* MakeMemOperate();

    /**
     * @brief 添加字段
     * @param strFieldName 字段名
     * @param strFieldValue 字段值
     * @param eFieldType 字段类型
     * @param bGroupBy 是否作为GroupBy字段
     * @param bOrderBy 是否作为OrderBy字段
     * @param strOrder OrederBy方式（DESC AESC）
     * @return 是否添加成功
     */
    virtual bool AddField(const std::string& strFieldName,
            const std::string& strFieldValue = "",
            E_COL_TYPE eFieldType = STRING,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");

    virtual bool AddField(const std::string& strFieldName, int32 iFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, uint32 uiFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, int64 llFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, uint64 ullFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, float fFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddField(const std::string& strFieldName, double dFieldValue,
            int iFieldOper = (FIELD_OPERATOR_DB | FIELD_OPERATOR_REDIS),
            const std::string& strColAs = "", bool bGroupBy = false,
            bool bOrderBy = false, const std::string& strOrder = "DESC");

private:
    Mydis* m_pMemRequest;
    uint32 m_uiSectionFactor;
};

} /* namespace neb */

#endif /* SRC_STORAGE_STORAGEOPERATOR_HPP_ */
