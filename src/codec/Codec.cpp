/*******************************************************************************
 * Project:  Nebula
 * @file     OssCodec.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月10日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Codec.hpp"

#include "util/encrypt/hconv.h"
#include "util/encrypt/rc5.h"

namespace neb
{

Codec::Codec(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel)
    : m_pLogger(pLogger), m_iErrno(0), m_eCodecType(eCodecType), m_pBindChannel(pBindChannel)
{
}

Codec::~Codec()
{
    LOG4_TRACE("codec_type %d", m_eCodecType);
}

bool Codec::Zip(const std::string& strSrc, std::string& strDest)
{
    /*
    int iErr = Z_OK;
    uLongf ulInLen = strSrc.size();
    uLongf ulOutBuffLen = strSrc.size();
    uLongf ulOutlen = 0;
    z_stream d_stream = { 0 }; // decompression stream
    Bytef* pInBuff = (Bytef*) malloc(ulInLen);
    Bytef* pOutBuff = (Bytef*) malloc(ulOutBuffLen);
    d_stream.zalloc = (alloc_func) Z_NULL;
    d_stream.zfree = (free_func) Z_NULL;
    d_stream.opaque = (voidpf) Z_NULL;
    d_stream.next_in = pInBuff;
    d_stream.avail_in = strSrc.size();
    d_stream.next_out = pOutBuff;
    d_stream.avail_out = ulOutBuffLen;

    if (deflateInit(&d_stream, Z_DEFAULT_COMPRESSION) != Z_OK)
    {
        return false;
    }

    memcpy(pInBuff, strSrc.c_str(), ulInLen);
    while (d_stream.avail_in != 0 && d_stream.total_out < ulOutBuffLen)
    {
        iErr = deflate(&d_stream, Z_NO_FLUSH);
        if (iErr != Z_OK)
        {
            iErr = deflateEnd(&d_stream);
            free(pInBuff);
            free(pOutBuff);
            return (false);
        }
    }

    if (d_stream.avail_in != 0) // 输出缓冲区不够
    {
        iErr = deflateEnd(&d_stream);
        free(pInBuff);
        free(pOutBuff);
        return (false);
    }

    iErr = deflate(&d_stream, Z_FINISH);

    iErr = deflateEnd(&d_stream);
    if (iErr != Z_OK)
    {
        free(pInBuff);
        free(pOutBuff);
        return (false);
    }

    strDest.assign((const char*) pOutBuff, d_stream.total_out);
    free(pInBuff);
    free(pOutBuff);
    */
    return (true);
}

bool Codec::Unzip(const std::string& strSrc, std::string& strDest)
{
    /*
    int iErr = Z_OK;
    uLongf ulInLen = strSrc.size();
    uLongf ulOutBuffLen = 1024000; // PB的Message最大支持1MB
    uLongf ulOutlen = 0;
    z_stream d_stream = { 0 }; // decompression stream
    Bytef* pInBuff = (Bytef*) malloc(ulInLen);
    Bytef* pOutBuff = (Bytef*) malloc(ulOutBuffLen);
    d_stream.zalloc = (alloc_func) Z_NULL;
    d_stream.zfree = (free_func) Z_NULL;
    d_stream.opaque = (voidpf) Z_NULL;
    d_stream.next_in = pInBuff;
    d_stream.avail_in = 0;
    d_stream.avail_out = ulOutBuffLen;
    d_stream.next_out = pOutBuff;

    if (inflateInit(&d_stream) != Z_OK)
    {
        return false;
    }

    memcpy(pInBuff, strSrc.c_str(), ulInLen);
    while (d_stream.total_out < ulOutBuffLen && d_stream.total_in < ulInLen)
    {
        d_stream.avail_in = d_stream.avail_out = 1; // force small buffers
        iErr = inflate(&d_stream, Z_NO_FLUSH);
        if (iErr == Z_STREAM_END)
        {
            break;
        }
        if (iErr != Z_OK)
        {
            iErr = inflateEnd(&d_stream);
            free(pInBuff);
            free(pOutBuff);
            return (false);
        }
    }

    iErr = inflateEnd(&d_stream);
    if (iErr != Z_OK)
    {
        free(pInBuff);
        free(pOutBuff);
        return (false);
    }

    strDest.assign((const char*) pOutBuff, d_stream.total_out);
    free(pInBuff);
    free(pOutBuff);
    */
    return (true);
}

bool Codec::Gzip(const std::string& strSrc, std::string& strDest)
{
    return(CodecUtil::Gzip(strSrc, strDest));
}

bool Codec::Gunzip(const std::string& strSrc, std::string& strDest)
{
    return(CodecUtil::Gunzip(strSrc, strDest));
}

bool Codec::Rc5Encrypt(const std::string& strSrc, std::string& strDest)
{
    return(false);
    /*
    rc5UserKey *pKey;
    rc5CBCAlg *pAlg;
    unsigned char szIv[20] = {"2015-08-10 08:53:47"};
    char* ucKey = new char[GetKey().size()];
    char* pPlain = new char[strSrc.size()];
    char* pCipher = new char[strSrc.size() * 2];
    int cipher_length = 0;
    int numBytesOut = 0;

    memcpy(ucKey, GetKey().c_str(), GetKey().size());
    memcpy(pPlain, strSrc.c_str(), strSrc.size());
    pKey = RC5_Key_Create();
    RC5_Key_Set(pKey, GetKey().size(), (unsigned char*)ucKey);

    pAlg = RC5_CBC_Create(1, 16, RC5_FIRST_VERSION, BB, szIv);
    (void) RC5_CBC_Encrypt_Init(pAlg, pKey);
    (void) RC5_CBC_Encrypt_Update(pAlg, strSrc.size(), (unsigned char*)pPlain,
                    &(numBytesOut), strSrc.size() * 2, (unsigned char*)pCipher);
    cipher_length += numBytesOut;
    (void) RC5_CBC_Encrypt_Final(pAlg, &(numBytesOut),
                    strSrc.size() * 2 - cipher_length, (unsigned char*)pCipher + cipher_length);
    cipher_length += numBytesOut;
    strDest.assign((const char*)pCipher, cipher_length);
    delete[] ucKey;
    delete[] pPlain;
    delete[] pCipher;
    RC5_Key_Destroy(pKey);
    RC5_CBC_Destroy(pAlg);
    return(true);
    */
}

bool Codec::Rc5Decrypt(const std::string& strSrc, std::string& strDest)
{
    return(false);
    /*
    rc5UserKey *pKey;
    rc5CBCAlg *pAlg;
    unsigned char szIv[20] = {"2015-08-10 08:53:47"};
    char* ucKey = new char[GetKey().size()];
    char* pCipher = new char[strSrc.size()];
    char* pPlain = new char[strSrc.size()];
    int plain_length = 0;
    int numBytesOut = 0;
    int fillBytes = 0;

    memcpy(ucKey, GetKey().c_str(), GetKey().size());
    memcpy(pCipher, strSrc.c_str(), strSrc.size());
    pKey = RC5_Key_Create();
    RC5_Key_Set(pKey, GetKey().size(), (unsigned char*)ucKey);

    pAlg = RC5_CBC_Create(1, 16, RC5_FIRST_VERSION, BB, szIv);
    (void) RC5_CBC_Decrypt_Init(pAlg, pKey);
    (void) RC5_CBC_Decrypt_Update(pAlg, strSrc.size(), (unsigned char*)pCipher,
                    &(numBytesOut), (unsigned char*)pPlain);
    fillBytes = (int)pPlain[numBytesOut - 1];
    plain_length += numBytesOut - fillBytes;
    strDest.assign((const char*)pPlain, plain_length);
    delete[] ucKey;
    delete[] pCipher;
    delete[] pPlain;
    RC5_Key_Destroy(pKey);
    RC5_CBC_Destroy(pAlg);
    return(true);
    */
}

bool Codec::AesEncrypt(const std::string& strSrc, std::string& strDest)
{
    return(CodecUtil::AesEncrypt(GetKey(), strSrc, strDest));
}

bool Codec::AesDecrypt(const std::string& strSrc, std::string& strDest)
{
    return(CodecUtil::AesDecrypt(GetKey(), strSrc, strDest));
}

} /* namespace neb */

