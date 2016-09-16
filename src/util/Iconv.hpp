/*******************************************************************************
* Project:  sdbl
* File:     Iconv.hpp
* Description: 字符集转换类
* Author:   bwarliao
* Created date:  2009-6-12
* Modify history:
*******************************************************************************/

#ifndef ICONV_HPP_
#define ICONV_HPP_

#include <errno.h>
#include <iconv.h>

namespace neb
{

class CIconv
{
    public:
        CIconv();
        CIconv(const char* tocode, const char* fromcode);
        virtual ~CIconv();

        int IconvOpen(const char* tocode, const char* fromcode);

        int Convert(char* inbuf, size_t inlen, char* outbuf, size_t outlen);

    private:
        iconv_t m_cd;
};

}

#endif /* ICONV_HPP_ */
