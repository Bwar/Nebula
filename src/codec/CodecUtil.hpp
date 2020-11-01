/*******************************************************************************
* Project:  Nebula
* @file     CodecUtil.hpp
* @brief 
* @author   Bwar
* @date:    2016年9月2日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_CODEC_CODECUTIL_HPP_
#define SRC_CODEC_CODECUTIL_HPP_

#include "Definition.hpp"

namespace neb
{

class CodecUtil
{
public:
    /**
     * @brief 用于进行判断主机字节序的联合体。
     * @note
          * 小端：低地址存放低字节，高地址存放高字节；
          * 大端：高地址存放低字节，低地址存放高字节；
          * 网络字节序是大端。
     */
    union UnionHostOrder
    {
        uint8  ucOrder[4];
        uint32 uiOrder;
    };

    CodecUtil();
    virtual ~CodecUtil();

    static uint16 N2H(uint16 unValue);
    static uint16 H2N(uint16 unValue);
    static uint32 N2H(uint32 uiValue);
    static uint32 H2N(uint32 uiValue);
    static uint64 N2H(uint64 ullValue);
    static uint64 H2N(uint64 ullValue);

protected:
    static inline bool IsLittleEndian()
    {
        return('L' == (uint8)s_uHostOrder.uiOrder);
    }

    static inline bool IsBigEndian()
    {
        return('B' == (uint8)s_uHostOrder.uiOrder);
    }

private:
    static UnionHostOrder s_uHostOrder;
};

}

#endif /* SRC_CODEC_CODECUTIL_HPP_ */
