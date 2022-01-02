/*******************************************************************************
 * Project:  Nebula
 * @file     Notations.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-05
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_TYPE_NOTATIONS_HPP_
#define SRC_TYPE_NOTATIONS_HPP_

#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Definition.hpp"
#include "util/CBuffer.hpp"
#include "codec/Codec.hpp"

namespace neb
{

struct InetAddr
{
    char type = 4;      // 4: ipv4,  6: ipv6
    uint32 addr[4] = {0};
};


/**
* @brief Bytes define
* @note if (iLength >= 0)  strValue length == iLength;
* if (iBytesCase < 0) the value represented is `null`.
*/
struct Bytes
{
    uint32 uiCapacity = 0;
    int32 iLength   = -1;
    char* pData = nullptr;
    Bytes(){}
    ~Bytes()
    {
        if (pData != nullptr)
        {
            free(pData);
            pData = nullptr;
        }
        uiCapacity = 0;
        iLength = -1;
    }
    Bytes(const Bytes&) = delete;
    Bytes(Bytes&& stBytes)
    {
        uiCapacity = stBytes.uiCapacity;
        iLength = stBytes.iLength;
        pData = stBytes.pData;
        stBytes.uiCapacity = 0;
        stBytes.iLength = -1;
        stBytes.pData = nullptr;
    }
    Bytes& operator=(const Bytes& stBytes)
    {
        if (stBytes.pData != nullptr && stBytes.iLength > 0)
        {
            pData = (char*)malloc(stBytes.uiCapacity);
            if (pData == nullptr)
            {
                iLength = -1;
            }
            else
            {
                uiCapacity = stBytes.uiCapacity;
                iLength = stBytes.iLength;
                memcpy(pData, stBytes.pData, stBytes.iLength);
            }
        }
        return(*this);
    }
    Bytes& operator=(Bytes&& stBytes)
    {
        uiCapacity = stBytes.uiCapacity;
        iLength = stBytes.iLength;
        pData = stBytes.pData;
        stBytes.uiCapacity = 0;
        stBytes.iLength = -1;
        stBytes.pData = nullptr;
        return(*this);
    }
    bool Assign(const char* pSrc, int32 iSrcLen)
    {
        if ((int)uiCapacity >= iSrcLen)
        {
            iLength = iSrcLen;
            memcpy(pData, pSrc, iSrcLen);
            return(true);
        }
        else
        {
            if (pData != nullptr)
            {
                free(pData);
            }
            pData = (char*)malloc(iSrcLen);
            if (pData == nullptr)
            {
                uiCapacity = 0;
                iLength = -1;
                return(false);
            }
            else
            {
                uiCapacity = iSrcLen;
                iLength = iSrcLen;
                memcpy(pData, pSrc, iSrcLen);
                return(true);
            }
        }
    }
    void Clear()
    {
        iLength = -1;
    }
};


/**
 * @brief Value define
 * @note if (iLength >= 0) strValue length == iLength;
 * if (iLength == -1) no byte should follow and the value represented is `null`.
 * if (iLength == -2) no byte should follow and the value represented is `not set` not resulting in any change to the existing value.
 * if (iLength < -2) is an invalid value and results in an error.
 */
typedef Bytes Value;

class Notations
{
public:
    Notations();
    virtual ~Notations();

    static void EncodeInt(int32 iValue, CBuffer* pBuff);
    static void EncodeLong(int64 llValue, CBuffer* pBuff);
    static void EncodeByte(uint8 ucValue, CBuffer* pBuff);
    static void EncodeShort(uint16 unValue, CBuffer* pBuff);
    static bool EncodeString(const std::string& strValue, CBuffer* pBuff);
    static bool EncodeLongString(const std::string& strValue, CBuffer* pBuff);
    static bool EncodeUuid(const std::string& strValue, CBuffer* pBuff);
    static bool EncodeStringList(const std::vector<std::string>& vecValue, CBuffer* pBuff);
    static bool EncodeBytes(const Bytes& stValue, CBuffer* pBuff);
    static bool EncodeValue(const Value& stValue, CBuffer* pBuff);
    static bool EncodeShortBytes(const Bytes& stValue, CBuffer* pBuff);
    static bool EncodeOption(uint16 unId, const Value& stValue, std::function<bool(uint16)> WithOptionValue, CBuffer* pBuff);
    static bool EncodeOptionList(const std::vector<std::pair<uint16, Value>>& vecValue, std::function<bool(uint16)> WithOptionValue, CBuffer* pBuff);
    static bool EncodeInet(const InetAddr& stAddr, int iPort, CBuffer* pBuff);
    static bool EncodeInetaddr(const InetAddr& stAddr, CBuffer* pBuff);
    static void EncodeConsistency(uint16 unValue, CBuffer* pBuff);
    static bool EncodeStringMap(const std::unordered_map<std::string, std::string>& mapValue, CBuffer* pBuff);
    static bool EncodeStringMultimap(const std::unordered_map<std::string, std::vector<std::string>>& mapValue, CBuffer* pBuff);
    static bool EncodeBytesMap(const std::unordered_map<std::string, Bytes>& mapValue, CBuffer* pBuff);

    static bool DecodeInt(CBuffer* pBuff, int32& iValue);
    static bool DecodeLong(CBuffer* pBuff, int64& llValue);
    static bool DecodeByte(CBuffer* pBuff, uint8& ucValue);
    static bool DecodeShort(CBuffer* pBuff, uint16& unValue);
    static bool DecodeString(CBuffer* pBuff, std::string& strValue);
    static bool DecodeLongString(CBuffer* pBuff, std::string& strValue);
    static bool DecodeUuid(CBuffer* pBuff, std::string& strValue);
    static bool DecodeStringList(CBuffer* pBuff, std::vector<std::string>& vecValue);
    static bool DecodeBytes(CBuffer* pBuff, Bytes& stValue);
    static bool DecodeValue(CBuffer* pBuff, Value& stValue);
    static bool DecodeShortBytes(CBuffer* pBuff, Bytes& stValue);
    static bool DecodeOption(CBuffer* pBuff, std::function<bool(uint16)> WithOptionValue, uint16& unId, Value& stValue);
    static bool DecodeOptionList(CBuffer* pBuff, std::function<bool(uint16)> WithOptionValue, std::vector<std::pair<uint16, Value>>& vecValue);
    static bool DecodeInet(CBuffer* pBuff, InetAddr& stAddr, int& iPort);
    static bool DecodeInetaddr(CBuffer* pBuff, InetAddr& stAddr);
    static bool DecodeConsistency(CBuffer* pBuff, uint16& unValue);
    static bool DecodeStringMap(CBuffer* pBuff, std::unordered_map<std::string, std::string>& mapValue);
    static bool DecodeStringMultimap(CBuffer* pBuff, std::unordered_map<std::string, std::vector<std::string>>& mapValue);
    static bool DecodeBytesMap(CBuffer* pBuff, std::unordered_map<std::string, Bytes>& mapValue);
};

} /* namespace neb */

#endif /* SRC_TYPE_NOTATIONS_HPP_ */

