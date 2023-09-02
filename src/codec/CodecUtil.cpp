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
#include <cryptopp/default.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/gzip.h>

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

bool CodecUtil::Gzip(const std::string& strSrc, std::string& strDest)
{
    try
    {
        CryptoPP::Gzip oGzipper;
        oGzipper.Put((CryptoPP::byte*)strSrc.c_str(), strSrc.size());
        oGzipper.MessageEnd();

        CryptoPP::word64 avail = oGzipper.MaxRetrievable();
        if(avail)
        {
            strDest.resize(avail);
            oGzipper.Get((CryptoPP::byte*)&strDest[0], strDest.size());
        }
    }
    catch(CryptoPP::InvalidDataFormat& e)
    {
        return(false);
    }
    return (true);
}

bool CodecUtil::Gunzip(const std::string& strSrc, std::string& strDest)
{
    try
    {
        CryptoPP::Gunzip oUnZipper;
        oUnZipper.Put((CryptoPP::byte*)strSrc.c_str(), strSrc.size());
        oUnZipper.MessageEnd();
        CryptoPP::word64 avail = oUnZipper.MaxRetrievable();
        if(avail)
        {
            strDest.resize(avail);
            oUnZipper.Get((CryptoPP::byte*)&strDest[0], strDest.size());
        }
    }
    catch(CryptoPP::InvalidDataFormat& e)
    {
        return(false);
    }
    return (true);
}

bool CodecUtil::AesEncrypt(const std::string& strKey, const std::string& strSrc, std::string& strDest)
{
    try
    {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption oAes;
        oAes.SetKeyWithIV((const CryptoPP::byte*)strKey.c_str(), 16, (const CryptoPP::byte*)"2015-08-10 08:53:47");
        CryptoPP::StreamTransformationFilter oEncryptor(
                        oAes, NULL, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        for (size_t i = 0; i < strSrc.size(); ++i)
        {
            oEncryptor.Put((CryptoPP::byte)strSrc[i]);
        }
        oEncryptor.MessageEnd();
        size_t length = oEncryptor.MaxRetrievable();
        strDest.resize(length, 0);
        oEncryptor.Get((CryptoPP::byte*)&strDest[0], length);
    }
    catch(CryptoPP::InvalidDataFormat& e)
    {
        return(false);
    }
    return(true);
}

bool CodecUtil::AesDecrypt(const std::string& strKey, const std::string& strSrc, std::string& strDest)
{
    try
    {
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption oAes;
        oAes.SetKeyWithIV((const CryptoPP::byte*)strKey.c_str(), 16, (const CryptoPP::byte*)"2015-08-10 08:53:47");
        CryptoPP::StreamTransformationFilter oDecryptor(
                        oAes, NULL, CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING);
        for (size_t i = 0; i < strSrc.size(); ++i)
        {
            oDecryptor.Put((CryptoPP::byte)strSrc[i]);
        }
        oDecryptor.MessageEnd();
    size_t length = oDecryptor.MaxRetrievable();
    strDest.resize(length, 0);
    oDecryptor.Get((CryptoPP::byte*)&strDest[0], length);
    }
    catch(CryptoPP::InvalidDataFormat& e)
    {
        return(false);
    }
    return(true);
}

}

