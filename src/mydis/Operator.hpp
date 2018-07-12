/*******************************************************************************
 * Project:  Nebula
 * @file     Operator.hpp
 * @brief    存储协议操作
 * @author   Bwar
 * @date:    2016年8月17日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_STORAGE_OPERATOR_HPP_
#define SRC_STORAGE_OPERATOR_HPP_

#include <stdio.h>
#include <string>
#include "pb/mydis.pb.h"
#include "Error.hpp"
#include "Definition.hpp"

namespace neb
{

/**
 * @brief 存储协议操作者
 */
class Operator
{
public:
    Operator();
    virtual ~Operator();

    virtual Mydis* MakeMemOperate() = 0;
};

} /* namespace neb */

#endif /* SRC_STORAGE_OPERATOR_HPP_ */
