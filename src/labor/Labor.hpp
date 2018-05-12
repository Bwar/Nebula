/*******************************************************************************
 * Project:  Nebula
 * @file     OssLabor.hpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月8日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef LABOR_LABOR_HPP_
#define LABOR_LABOR_HPP_

#include <ctime>
#include <string>
#include "pb/msg.pb.h"
#include "Definition.hpp"

namespace neb
{

class Actor;

class Labor
{
public:
    Labor(){};
    Labor(const Labor&) = delete;
    Labor& operator=(const Labor&) = delete;
    virtual ~Labor(){};

public:
    virtual uint32 GetNodeId() const = 0;
    virtual time_t GetNowTime() const = 0;
    virtual const std::string& GetNodeIdentify() const = 0;
    virtual bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender = nullptr) = 0;
};

} /* namespace neb */

#endif /* LABOR_LABOR_HPP_ */
