/*******************************************************************************
* Project:  Nebula
* @file     CodecUtil.cpp
* @brief 
* @author   Bwar
* @date:    2016年9月2日
* @note
* Modify history:
******************************************************************************/
#include "CodecUtil.hpp"

namespace neb
{

CodecUtil::UnionHostOrder CodecUtil::s_uHostOrder = { { 'L', '?', '?', 'B' } };

CodecUtil::CodecUtil()
{
}

CodecUtil::~CodecUtil()
{
}

uint16 CodecUtil::N2H(uint16 unValue)
{
    if (IsLittleEndian())
    {
        return((unValue << 8) | (unValue >> 8));
    }
    return(unValue);
}

uint16 CodecUtil::H2N(uint16 unValue)
{
    if (IsLittleEndian())
    {
        return((unValue << 8) | (unValue >> 8));
    }
    return(unValue);
}

uint32 CodecUtil::N2H(uint32 uiValue)
{
    if (IsLittleEndian())
    {
        return(((uiValue             ) << 24)
                | ((uiValue & 0x0000FF00) <<  8)
                | ((uiValue & 0x00FF0000) >>  8)
                | ((uiValue             ) >> 24));
    }
    return(uiValue);
}

uint32 CodecUtil::H2N(uint32 uiValue)
{
    if (IsLittleEndian())
    {
        return(((uiValue             ) << 24)
                | ((uiValue & 0x0000FF00) <<  8)
                | ((uiValue & 0x00FF0000) >>  8)
                | ((uiValue             ) >> 24));
    }
    return uiValue;
}

uint64 CodecUtil::N2H(uint64 ullValue)
{
    if (IsLittleEndian())
    {
        return(((ullValue                     ) << 56)
                | ((ullValue & 0x000000000000FF00) << 40)
                | ((ullValue & 0x0000000000FF0000) << 24)
                | ((ullValue & 0x00000000FF000000) <<  8)
                | ((ullValue & 0x000000FF00000000) >>  8)
                | ((ullValue & 0x0000FF0000000000) >> 24)
                | ((ullValue & 0x00FF000000000000) >> 40)
                | ((ullValue                     ) >> 56));
    }
    return(ullValue);
}

uint64 CodecUtil::H2N(uint64 ullValue)
{
    if (IsLittleEndian())
    {
        return(((ullValue                     ) << 56)
                | ((ullValue & 0x000000000000FF00) << 40)
                | ((ullValue & 0x0000000000FF0000) << 24)
                | ((ullValue & 0x00000000FF000000) <<  8)
                | ((ullValue & 0x000000FF00000000) >>  8)
                | ((ullValue & 0x0000FF0000000000) >> 24)
                | ((ullValue & 0x00FF000000000000) >> 40)
                | ((ullValue                     ) >> 56));
    }
    return(ullValue);
}

}

