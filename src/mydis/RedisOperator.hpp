/*******************************************************************************
 * Project:  Nebula
 * @file     RedisOperator.hpp
 * @brief    Redis操作协议打包
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STORAGE_REDISOPERATOR_HPP_
#define SRC_STORAGE_REDISOPERATOR_HPP_

#include "Operator.hpp"

namespace neb
{
/**
 * @brief StarMydis Redis请求协议生成器
 * @note 存储请求协议生成，用于单独请求Redis的场景。
 */
class RedisOperator: public Operator
{
public:
    /**
     * @brief 存储请求协议生成
     * @note 存储请求协议生成，用于同时单独请求Redis的场景。
     * @param uiSectionFactor   hash分段因子
     * @param strRedisKey       Redis key
     * @param strWriteCmd       Redis写命令（若是读，写命令用于Redis未命中，DataProxy从DB中读取到数据写回Redis时使用）
     * @param strReadCmd        Redis读命令（若是写，读命令为""空串）
     */
    RedisOperator(
            uint32 uiSectionFactor,
            const std::string& strRedisKey,
            const std::string& strWriteCmd,
            const std::string& strReadCmd = "");
    virtual ~RedisOperator();

    virtual Mydis* MakeMemOperate();

    /**
     * @brief 添加字段
     * @note 当写操作Redis与数据库存储的数据字段不相等且Redis Field数量为1或Redis第一个Field的value为空时，
     * 生成的协议包支持，将数据库协议里所有field存储到Redis的Field Value中。详见MemOperator::MakeMemOperate()函数实现。
     * @param strFieldName 字段名（当不是hash结果，是list，set等结构时填写strFieldValue，不填写strFieldName）
     * @param strFieldValue 字段值
     * @return 是否添加成功
     */
    virtual bool AddRedisField(const std::string& strFieldName, const std::string& strFieldValue = "");

    virtual bool AddRedisField(const std::string& strFieldName, int32 iFieldValue);
    virtual bool AddRedisField(const std::string& strFieldName, uint32 uiFieldValue);
    virtual bool AddRedisField(const std::string& strFieldName, int64 llFieldValue);
    virtual bool AddRedisField(const std::string& strFieldName, uint64 ullFieldValue);
    virtual bool AddRedisField(const std::string& strFieldName, float fFieldValue);
    virtual bool AddRedisField(const std::string& strFieldName, double dFieldValue);

protected:
    Mydis::RedisOperate* GetRedisOperate()
    {
        return(m_pRedisOperate);
    }

    void SetRedisOperateNull()
    {
        m_pRedisOperate = NULL;
    }

private:
    Mydis* m_pRedisMemRequest;
    Mydis::RedisOperate* m_pRedisOperate;
    uint32 m_uiSectionFactor;
};

} /* namespace neb */

#endif /* SRC_STORAGE_REDISOPERATOR_HPP_ */
