/*******************************************************************************
 * Project:  Nebula
 * @file     StepIoTimeout2.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月30日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_OBJECT_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_
#define SRC_OBJECT_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_

#include "object/step/PbStep.hpp"

namespace neb
{

/**
 * @brief IO超时回调步骤
 * @note 当发生正常连接（连接成功后曾经发送过合法数据包）IO超时事件时，创建一个StepIoTimeout，
 * 若该步骤正常回调，则重置连接超时，若该步骤超时，则关闭连接，销毁连接资源。该步骤实现的是服务
 * 端发起的心跳机制，心跳时间间隔就是IO超时时间。
 */
class StepIoTimeout: public PbStep
{
public:
    StepIoTimeout(const tagChannelContext& stCtx);
    virtual ~StepIoTimeout();

    virtual E_CMD_STATUS Emit(
            int iErrno = 0,
            const std::string& strErrMsg = "",
            void* data = NULL);

    virtual E_CMD_STATUS Callback(
            const tagChannelContext& stCtx,
            const MsgHead& oInMsgHead,
            const MsgBody& oInMsgBody,
            void* data = NULL);

    virtual E_CMD_STATUS Timeout();

    virtual std::string ObjectName() const
    {
        return("StepIoTimeout");
    }

private:
    tagChannelContext m_stCtx;
};

} /* namespace neb */

#endif /* SRC_OBJECT_STEP_SYS_STEP_STEPIOTIMEOUT_HPP_ */
