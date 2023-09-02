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

#include <string>
#include "Definition.hpp"

namespace neb
{

enum E_COMPRESSION
{
    COMPRESS_NA             = 0, // not compress
    COMPRESS_GZIP           = 1,
    COMPRESS_DEFLATE        = 2,
    COMPRESS_SNAPPY         = 3,
};

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

    static bool Gzip(const std::string& strSrc, std::string& strDest);
    static bool Gunzip(const std::string& strSrc, std::string& strDest);
    //static bool Deflate(const std::string& strSrc, std::string& strDest);
    //static bool Undeflate(const std::string& strSrc, std::string& strDest);
    static bool AesEncrypt(const std::string& strKey, const std::string& strSrc, std::string& strDest);
    static bool AesDecrypt(const std::string& strKey, const std::string& strSrc, std::string& strDest);

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
