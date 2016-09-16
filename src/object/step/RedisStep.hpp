/*******************************************************************************
 * Project:  Nebula
 * @file     RedisStep.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_REDISSTEP_HPP_
#define SRC_OBJECT_STEP_REDISSTEP_HPP_

#include <set>
#include <list>
#include "storage/dbi/redis/RedisCmd.hpp"
#include "Step.hpp"

namespace neb
{

/**
 * @brief StepRedis在回调后一定会被删除
 */
class RedisStep: public Step
{
public:
    RedisStep(Step* pNextStep = NULL);
    RedisStep(const tagChannelContext& stCtx, const MsgHead& oReqMsgHead, const MsgBody& oReqMsgBody, Step* pNextStep = NULL);
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

public:
    /**
     * @brief 超时回调
     * @note redis step 暂时不启用超时机制
     * @return 回调状态
     */
    virtual E_CMD_STATUS Timeout(){return(CMD_STATUS_FAULT);}

public:
    RedisCmd* MutableRedisCmd()
    {
        return(m_pRedisCmd);
    }

public:
    const RedisCmd* GetRedisCmd()
    {
        return(m_pRedisCmd);
    }

protected:  // 请求端的上下文信息，通过Step构造函数初始化，若调用的是不带参数的构造函数Step()，则这几个成员不会被初始化
    tagChannelContext m_stCtx;
    MsgHead m_oReqMsgHead;
    MsgBody m_oReqMsgBody;

private:
    RedisCmd* m_pRedisCmd;
};

/**
 * @brief Redis连接属性
 * @note  Redis连接属性，因内部带有许多指针，并且没有必要提供深拷贝构造，所以不可以拷贝，也无需拷贝
 */
struct tagRedisAttr
{
    uint32 ulSeq;                           ///< redis连接序列号
    bool bIsReady;                          ///< redis连接是否准备就绪
    std::list<RedisStep*> listData;         ///< redis连接回调数据
    std::list<RedisStep*> listWaitData;     ///< redis等待连接成功需执行命令的数据

    tagRedisAttr() : ulSeq(0), bIsReady(false)
    {
    }

    ~tagRedisAttr()
    {
        //freeReplyObject(pReply);  redisProcessCallbacks()函数中有自动回收

        for (std::list<RedisStep*>::iterator step_iter = listData.begin();
                        step_iter != listData.end(); ++step_iter)
        {
            if (*step_iter != NULL)
            {
                DELETE(*step_iter);
            }
        }
        listData.clear();

        for (std::list<RedisStep*>::iterator step_iter = listWaitData.begin();
                        step_iter != listWaitData.end(); ++step_iter)
        {
            if (*step_iter != NULL)
            {
                DELETE(*step_iter);
            }
        }
        listWaitData.clear();
    }
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_REDISSTEP_HPP_ */
