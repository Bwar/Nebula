/*******************************************************************************
* Project:  sdbl
* File:     Iconv.cpp
* Description: 字符集转换类
* Author:   bwarliao
* Created date:  2009-6-12
* Modify history:
*******************************************************************************/

#include <string.h>
#include "Iconv.hpp"

namespace neb
{

CIconv::CIconv()
    : m_cd(0)
{
}

CIconv::CIconv(const char *tocode, const char *fromcode)
{
    IconvOpen(tocode, fromcode);
}

CIconv::~CIconv()
{
    iconv_close(m_cd);
}

int CIconv::IconvOpen(const char *tocode, const char *fromcode)
{
    m_cd = iconv_open(tocode, fromcode);
    if (m_cd == 0)
    {
        return errno;
    }
    return 0;
}

int CIconv::Convert(char* inbuf, size_t inlen, char* outbuf, size_t outlen)
{
    memset(outbuf,0,outlen);
    char *pin = const_cast<char *>(inbuf);
    char *pout = const_cast<char *>(outbuf);

    if (iconv(m_cd, &pin, &inlen, &pout, &outlen) == -1)
    {
        return errno;
    }
    return 0;
}

}
