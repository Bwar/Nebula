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

#include "Definition.hpp"

namespace neb
{

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
};

} /* namespace neb */

#endif /* LABOR_LABOR_HPP_ */
