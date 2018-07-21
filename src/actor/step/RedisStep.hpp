/*******************************************************************************
 * Project:  Nebula
 * @file     RedisStep.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_REDISSTEP_HPP_
#define SRC_ACTOR_STEP_REDISSTEP_HPP_

#include <set>
#include <list>
#include "actor/step/Step.hpp"

namespace neb
{

/**
 * @brief StepRedis在回调后一定会被删除
 */
class RedisStep: public Step
{
public:
    RedisStep(std::shared_ptr<Step> pNextStep = nullptr);
    RedisStep(const RedisStep&) = delete;
    RedisStep& operator=(const RedisStep&) = delete;
    virtual ~RedisStep();

    /**
     * @brief redis步骤回调
     * @param c redis连接上下文
     * @param status 回调状态
     * @param pReply 执行结果集
     */
    virtual E_CMD_STATUS Callback(
                    const redisAsyncContext *c,
                    int status,
                    redisReply* pReply) = 0;

    /**
     * @brief redis超时
     * @note 此函数暂时无用，不会有redis超时回调
     */
    virtual E_CMD_STATUS Timeout()
    {
        return(CMD_STATUS_FAULT);
    }

public:
    /**
     * @brief 设置redis命令
     */
    void SetCmd(const std::string& strCmd);

    /**
     * @brief 设置redis命令参数
     * @note redis命令后面的key也认为是参数之一
     */
    void Append(const std::string& strArgument, bool bIsBinaryArg = false);

    /**
     * @brief 设置用户hash定位redis节点的字符串
     * @note 所设置的字符串不是redis的key，key和参数一起通过Append()添加
     */
    void SetHashKey(const std::string& strHashKey);

    std::string CmdToString() const;

    const std::string& GetErrMsg() const
    {
        return(m_strErr);
    }

    const std::string& GetCmd() const
    {
        return(m_strCmd);
    }

    const std::string& GetHashKey() const
    {
        return(m_strHashKey);
    }

    const std::vector<std::pair<std::string, bool> >& GetCmdArguments() const
    {
        return(m_vecCmdArguments);
    }

private:
    std::string m_strErr;
    std::string m_strHashKey;
    std::string m_strCmd;
    std::vector<std::pair<std::string, bool> > m_vecCmdArguments;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_REDISSTEP_HPP_ */
