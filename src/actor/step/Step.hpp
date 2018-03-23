/*******************************************************************************
 * Project:  Nebula
 * @file     Step.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_STEP_HPP_
#define SRC_ACTOR_STEP_STEP_HPP_

#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Step: public Actor
{
public:
    Step(ACTOR_TYPE eObjectType, ev_tstamp dTimeout, Step* pNextStep = NULL);
    Step(const Step&) = delete;
    Step& operator=(const Step&) = delete;
    virtual ~Step();

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。 Emit()大部分情况下不需要传递参数，
     * 三个带缺省值的参数是为了让上一个通用Step执行出错时将错误码带入下一步业务逻辑Step，由具体
     * 业务逻辑处理。
     * @param iErrno  错误码
     * @param strErrMsg 错误信息
     * @param data 数据指针，有专用数据时使用
     * @return 执行状态
     */
    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    void AddNextStepSeq(Step* pStep);
    void AddPreStepSeq(Step* pStep);

private:
    std::set<uint32> m_setNextStepSeq;
    std::set<uint32> m_setPreStepSeq;

    friend class WorkerImpl;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_STEP_HPP_ */
