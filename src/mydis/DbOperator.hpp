/*******************************************************************************
 * Project:  Nebula
 * @file     DbOperator.hpp
 * @brief    数据库操作协议打包
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STORAGE_DBOPERATOR_HPP_
#define SRC_STORAGE_DBOPERATOR_HPP_

#include "Operator.hpp"

namespace neb
{

/**
 * @brief StarMydis数据库请求协议生成器
 * @note 存储请求协议生成，用于单独请求数据库的场景。
 */
class DbOperator: public Operator
{
public:
    /**
     * @brief 创建数据库表操作类
     * @param uiSectionFactor   hash分段因子
     * @param strTableName      表名
     * @param eQueryType        查询类型（SELECT，INSERT，REPLACE等）
     * @param uiModFactor       分表取模因子，当该参数为0时使用uiSectionFactor
     */
    DbOperator(
            uint32 uiSectionFactor,
            const std::string& strTableName,
            Mydis::DbOperate::E_QUERY_TYPE eQueryType,
            uint32 uiModFactor = 0);
    virtual ~DbOperator();

    virtual Mydis* MakeMemOperate();

    /**
     * @brief 添加字段
     * @param strFieldName 字段名
     * @param strFieldValue 字段值
     * @param eFieldType 字段类型
     * @param strColAs as字段名
     * @param bGroupBy 是否作为GroupBy字段
     * @param bOrderBy 是否作为OrderBy字段
     * @param strOrder OrederBy方式（DESC AESC）
     * @return 是否添加成功
     */
    virtual bool AddDbField(const std::string& strFieldName, const std::string& strFieldValue = "",
                    E_COL_TYPE eFieldType = STRING,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");

    virtual bool AddDbField(const std::string& strFieldName, int32 iFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddDbField(const std::string& strFieldName, uint32 uiFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddDbField(const std::string& strFieldName, int64 llFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddDbField(const std::string& strFieldName, uint64 ullFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddDbField(const std::string& strFieldName, float fFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");
    virtual bool AddDbField(const std::string& strFieldName, double dFieldValue,
                    const std::string& strColAs = "",
                    bool bGroupBy = false, bool bOrderBy = false, const std::string& strOrder = "DESC");

    /**
     * @brief 添加查询条件
     * @note 适用于没有优先级无须分组的条件添加，每一个添加的条件会按添加的先后顺序依次展开
     * @param eRelation 条件关系
     * @param strFieldName 条件字段名
     * @param strFieldValue 条件字段值
     * @param eFieldType 条件字段类型
     * @param strRightFieldName 右值条件关系字段
     * @return 是否添加成功
     */
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::string& strFieldValue,
                    E_COL_TYPE eFieldType = STRING, const std::string& strRightFieldName = "");

    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, int32 iFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, uint32 uiFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, int64 llFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, uint64 ullFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, float fFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, double dFieldValue,
                    const std::string& strRightFieldName = "");
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::vector<uint32>& vecFieldValues);
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::vector<uint64>& vecFieldValues);
    virtual bool AddCondition(Mydis::DbOperate::Condition::E_RELATION eRelation,
                    const std::string& strFieldName, const std::vector<std::string>& vecFieldValues);

    /**
     * @brief 添加条件到指定条件组合中
     * @note 添加条件到指定条件组合，WHERE (col1 = 2 OR col1 = 3) AND (col2 > 80 AND col3 = 1)中
     * 条件组(col1 = 2 OR col1 = 3)的ID为0，条件组内部关系为OR；条件组(col2 > 80 AND col3 = 1)的ID为1，
     * 条件组内部关系为AND
     * @param iGroupIdx 条件组合ID，0表示第一个条件组，1表示第二个条件组
     * @param eGroupRelation 条件组内部各条件之间的关系（AND 或者 OR）
     * @param eRelation 条件关系
     * @param strFieldName 条件字段名
     * @param strFieldValue 条件字段值
     * @param eFieldType 条件字段类型
     * @param strRightFieldName 右值条件关系字段
     * @return 是否添加成功
     */
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, const std::string& strFieldValue,
                        E_COL_TYPE eFieldType = STRING, const std::string& strRightFieldName = "");

    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, int32 iFieldValue,
                        const std::string& strRightFieldName = "");
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, uint32 uiFieldValue,
                        const std::string& strRightFieldName = "");
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, int64 llFieldValue,
                        const std::string& strRightFieldName = "");
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, uint64 ullFieldValue,
                        const std::string& strRightFieldName = "");
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, float fFieldValue,
                        const std::string& strRightFieldName = "");
    virtual bool AddCondition(int iGroupIdx,
                        Mydis::DbOperate::ConditionGroup::E_RELATION eGroupRelation,
                        Mydis::DbOperate::Condition::E_RELATION eRelation,
                        const std::string& strFieldName, double dFieldValue,
                        const std::string& strRightFieldName = "");

    /**
     * @brief 设置条件组间关系
     * @param eRelation 条件组间关系（AND 或者 OR）
     */
    virtual void SetConditionGroupRelation(Mydis::DbOperate::ConditionGroup::E_RELATION eRelation);

    /**
     * @brief 添加limit记录数限制
     * @param uiLimitFrom 从哪条记录来时限制
     * @param uiLimit 记录条数
     * @note 添加记录数限制，AddLimit(100)时， limit 10; AddLimit(100, 200)时，limit 100,200。
     */
    virtual void AddLimit(unsigned int uiLimitFrom, unsigned int uiLimit);
    virtual void AddLimit(unsigned int uiLimit);

protected:
    Mydis::DbOperate* GetDbOperate()
    {
        return(m_pDbOperate);
    }

    void SetDbOperateNull()
    {
        m_pDbOperate = NULL;
    }

private:
    Mydis* m_pDbMemRequest;
    Mydis::DbOperate* m_pDbOperate;
    uint32 m_uiSectionFactor;
};

} /* namespace neb */

#endif /* SRC_STORAGE_DBOPERATOR_HPP_ */
