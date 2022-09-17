/*******************************************************************************
 * Project:  Nebula
 * @file     Notations.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-05
 * @note     
 * Modify history:
 ******************************************************************************/
#include "Notations.hpp"

namespace neb
{

Notations::Notations()
{
}

Notations::~Notations()
{
}

void Notations::EncodeInt(int32 iValue, CBuffer* pBuff)
{
    uint32 uiData = CodecUtil::H2N(uint32(iValue));
    pBuff->Write(&uiData, 4);
}

void Notations::EncodeLong(int64 llValue, CBuffer* pBuff)
{
    uint64 ullData = CodecUtil::H2N(uint64(llValue));
    pBuff->Write(&ullData, 8);
}

void Notations::EncodeByte(uint8 ucValue, CBuffer* pBuff)
{
    pBuff->WriteByte((char)ucValue);
}

void Notations::EncodeShort(uint16 unValue, CBuffer* pBuff)
{
    uint16 unData = CodecUtil::H2N(unValue);
    pBuff->Write(&unData, 2);
}

bool Notations::EncodeString(const std::string& strValue, CBuffer* pBuff)
{
    if (strValue.size() > 65535)
    {
        return(false);
    }
    else
    {
        EncodeShort((uint16)strValue.size(), pBuff);
        pBuff->Write(strValue.data(), strValue.size());
        return(true);
    }
}

bool Notations::EncodeString(const char* szValue, uint32 uiSize, CBuffer* pBuff)
{
    if (uiSize > 65535)
    {
        return(false);
    }
    else
    {
        EncodeShort((uint16)uiSize, pBuff);
        pBuff->Write(szValue, uiSize);
        return(true);
    }
}

bool Notations::EncodeLongString(const std::string& strValue, CBuffer* pBuff)
{
    if (strValue.size() > 2147483647)
    {
        return(false);
    }
    else
    {
        EncodeInt((int32)strValue.size(), pBuff);
        pBuff->Write(strValue.data(), strValue.size());
        return(true);
    }
}

bool Notations::EncodeLongString(const char* szValue, uint32 uiSize, CBuffer* pBuff)
{
    if (uiSize > 2147483647)
    {
        return(false);
    }
    else
    {
        EncodeInt((int32)uiSize, pBuff);
        pBuff->Write(szValue, uiSize);
        return(true);
    }
}

bool Notations::EncodeUuid(const std::string& strValue, CBuffer* pBuff)
{
    if (strValue.size() != 16)
    {
        return(false);
    }
    pBuff->Write(strValue.data(), strValue.size());
    return(true);
}

bool Notations::EncodeStringList(const std::vector<std::string>& vecValue, CBuffer* pBuff)
{
    if (vecValue.size() > 65535)
    {
        return(false);
    }
    EncodeShort(vecValue.size(), pBuff);
    for (uint16 i = 0; i < vecValue.size(); ++i)
    {
        if (!EncodeString(vecValue[i], pBuff))
        {
            return(false);
        }
    }
    return(true);
}

bool Notations::EncodeBytes(const Bytes& stValue, CBuffer* pBuff)
{
    if (stValue.iLength < 0)
    {
        EncodeInt(stValue.iLength, pBuff);
        return(true);
    }
    else if (stValue.iLength > 2147483647)
    {
        return(false);
    }
    else
    {
        EncodeInt(stValue.iLength, pBuff);
        pBuff->Write(stValue.pData, stValue.iLength);
        return(true);
    }
}

bool Notations::EncodeBytes(const char* szValue, int32 iSize, CBuffer* pBuff)
{
    if (iSize < 0)
    {
        EncodeInt(iSize, pBuff);
        return(true);
    }
    else if (iSize > 2147483647)
    {
        return(false);
    }
    else
    {
        EncodeInt(iSize, pBuff);
        pBuff->Write(szValue, iSize);
        return(true);
    }
}

bool Notations::EncodeValue(const Value& stValue, CBuffer* pBuff)
{
    return(EncodeBytes(stValue, pBuff));
}

bool Notations::EncodeShortBytes(const Bytes& stValue, CBuffer* pBuff)
{
    if (stValue.iLength > 65535)
    {
        return(false);
    }
    else
    {
        EncodeShort((uint16)stValue.iLength, pBuff);
        pBuff->Write(stValue.pData, stValue.iLength);
        return(true);
    }
}

bool Notations::EncodeOption(uint16 unId, const Value& stValue, std::function<bool(uint16)> WithOptionValue, CBuffer* pBuff)
{
    EncodeShort(unId, pBuff);
    if (WithOptionValue(unId))
    {
        return(EncodeValue(stValue, pBuff));
    }
    return(true);
}

bool Notations::EncodeOptionList(const std::vector<std::pair<uint16, Value>>& vecValue, std::function<bool(uint16)> WithOptionValue, CBuffer* pBuff)
{
    if (vecValue.size() > 65535)
    {
        return(false);
    }
    EncodeShort(vecValue.size(), pBuff);
    for (uint16 i = 0; i < vecValue.size(); ++i)
    {
        if (!EncodeOption(vecValue[i].first, vecValue[i].second, WithOptionValue, pBuff))
        {
            return(false);
        }
    }
    return(true);
}

bool Notations::EncodeInet(const InetAddr& stAddr, int iPort, CBuffer* pBuff)
{
    if (EncodeInetaddr(stAddr, pBuff))
    {
        EncodeInt(iPort, pBuff);
        return(true);
    }
    return(false);
}

bool Notations::EncodeInetaddr(const InetAddr& stAddr, CBuffer* pBuff)
{
    if (stAddr.type == 4)
    {
        EncodeByte(stAddr.type, pBuff);
        uint32 uiAddr = CodecUtil::H2N(stAddr.addr[0]);
        pBuff->Write(&uiAddr, 4);
        return(true);
    }
    else if (stAddr.type == 16)
    {
        EncodeByte(stAddr.type, pBuff);
        uint32 uiAddr = CodecUtil::H2N(stAddr.addr[0]);
        pBuff->Write(&uiAddr, 4);
        uiAddr = CodecUtil::H2N(stAddr.addr[1]);
        pBuff->Write(&uiAddr, 4);
        uiAddr = CodecUtil::H2N(stAddr.addr[2]);
        pBuff->Write(&uiAddr, 4);
        uiAddr = CodecUtil::H2N(stAddr.addr[3]);
        pBuff->Write(&uiAddr, 4);
        return(true);
    }
    else
    {
        return(false);
    }
}

void Notations::EncodeConsistency(uint16 unValue, CBuffer* pBuff)
{
    EncodeShort(unValue, pBuff);
}

bool Notations::EncodeStringMap(const std::unordered_map<std::string, std::string>& mapValue, CBuffer* pBuff)
{
    if (mapValue.size() > 65535)
    {
        return(false);
    }
    EncodeShort(mapValue.size(), pBuff);
    for (auto iter = mapValue.begin(); iter != mapValue.end(); ++iter)
    {
        if (!EncodeString(iter->first, pBuff)
                || !EncodeString(iter->second, pBuff))
        {
            return(false);
        }
    }
    return(true);
}

bool Notations::EncodeStringMultimap(const std::unordered_map<std::string, std::vector<std::string>>& mapValue, CBuffer* pBuff)
{
    if (mapValue.size() > 65535)
    {
        return(false);
    }
    EncodeShort(mapValue.size(), pBuff);
    for (auto iter = mapValue.begin(); iter != mapValue.end(); ++iter)
    {
        if (!EncodeString(iter->first, pBuff)
                || !EncodeStringList(iter->second, pBuff))
        {
            return(false);
        }
    }
    return(true);
}

bool Notations::EncodeBytesMap(const std::unordered_map<std::string, Bytes>& mapValue, CBuffer* pBuff)
{
    if (mapValue.size() > 65535)
    {
        return(false);
    }
    EncodeShort(mapValue.size(), pBuff);
    for (auto iter = mapValue.begin(); iter != mapValue.end(); ++iter)
    {
        if (!EncodeString(iter->first, pBuff)
                || !EncodeBytes(iter->second, pBuff))
        {
            return(false);
        }
    }
    return(true);
}

bool Notations::DecodeInt(CBuffer* pBuff, int32& iValue)
{
    if (pBuff->ReadableBytes() < 4)
    {
        return(false);
    }
    uint32 uiData;
    pBuff->Read(&uiData, 4);
    iValue = CodecUtil::N2H(uiData);
    return(true);
}

bool Notations::DecodeLong(CBuffer* pBuff, int64& llValue)
{
    if (pBuff->ReadableBytes() < 8)
    {
        return(false);
    }
    uint64 ullData;
    pBuff->Read(&ullData, 8);
    llValue = CodecUtil::N2H(ullData);
    return(true);
}

bool Notations::DecodeByte(CBuffer* pBuff, uint8& ucValue)
{
    if (pBuff->ReadableBytes() < 1)
    {
        return(false);
    }
    pBuff->Read(&ucValue, 1);
    return(true);
}

bool Notations::DecodeShort(CBuffer* pBuff, uint16& unValue)
{
    if (pBuff->ReadableBytes() < 2)
    {
        return(false);
    }
    uint16 unData;
    pBuff->Read(&unData, 2);
    unValue = CodecUtil::N2H(unData);
    return(true);
}

bool Notations::DecodeString(CBuffer* pBuff, std::string& strValue)
{
    uint16 unLength;
    if (DecodeShort(pBuff, unLength))
    {
        if (pBuff->ReadableBytes() < unLength)
        {
            return(false);
        }
        strValue.assign(pBuff->GetRawReadBuffer(), unLength);
        pBuff->AdvanceReadIndex(unLength);
        return(true);
    }
    return(false);
}

bool Notations::DecodeLongString(CBuffer* pBuff, std::string& strValue)
{
    int32 iLength;
    if (DecodeInt(pBuff, iLength))
    {
        if ((int32)pBuff->ReadableBytes() < iLength)
        {
            return(false);
        }
        strValue.assign(pBuff->GetRawReadBuffer(), iLength);
        pBuff->AdvanceReadIndex(iLength);
        return(true);
    }
    return(false);
}

bool Notations::DecodeUuid(CBuffer* pBuff, std::string& strValue)
{
    if (pBuff->ReadableBytes() < 16)
    {
        return(false);
    }
    strValue.assign(pBuff->GetRawReadBuffer(), 16);
    pBuff->AdvanceReadIndex(16);
    return(true);
}

bool Notations::DecodeStringList(CBuffer* pBuff, std::vector<std::string>& vecValue)
{
    uint16 unNum;
    if (DecodeShort(pBuff, unNum))
    {
        vecValue.clear();
        std::string strValue;
        for (uint16 i = 0; i < unNum; ++i)
        {
            if (!DecodeString(pBuff, strValue))
            {
                return(false);
            }
            vecValue.push_back(strValue);
        }
        return(true);
    }
    return(false);
}

bool Notations::DecodeBytes(CBuffer* pBuff,  Bytes& stValue)
{
    if (DecodeInt(pBuff, stValue.iLength))
    {
        if (stValue.iLength < 0)
        {
            stValue.Clear();
            return(true);
        }
        if ((int32)pBuff->ReadableBytes() < stValue.iLength)
        {
            stValue.Clear();
            return(false);
        }
        stValue.Assign(pBuff->GetRawReadBuffer(), stValue.iLength);
        pBuff->AdvanceReadIndex(stValue.iLength);
        return(true);
    }
    return(false);
}

bool Notations::DecodeValue(CBuffer* pBuff, Value& stValue)
{
    if (DecodeInt(pBuff, stValue.iLength))
    {
        if (stValue.iLength < 0)
        {
            return(true);
        }
        if ((int32)pBuff->ReadableBytes() < stValue.iLength)
        {
            stValue.Clear();
            return(false);
        }
        stValue.Assign(pBuff->GetRawReadBuffer(), stValue.iLength);
        pBuff->AdvanceReadIndex(stValue.iLength);
        return(true);
    }
    return(false);
}

bool Notations::DecodeShortBytes(CBuffer* pBuff, Bytes& stValue)
{
    uint16 unLength;
    if (DecodeShort(pBuff, unLength))
    {
        if (pBuff->ReadableBytes() < unLength)
        {
            return(false);
        }
        stValue.iLength = (int32)unLength;
        stValue.Assign(pBuff->GetRawReadBuffer(), stValue.iLength);
        pBuff->AdvanceReadIndex(unLength);
        return(true);
    }
    return(false);
}

bool Notations::DecodeOption(CBuffer* pBuff, std::function<bool(uint16)> WithOptionValue, uint16& unId, Value& stValue)
{
    if (DecodeShort(pBuff, unId))
    {
        if (WithOptionValue(unId))
        {
            return(DecodeValue(pBuff, stValue));
        }
        return(true);
    }
    return(false);
}

bool Notations::DecodeOptionList(CBuffer* pBuff, std::function<bool(uint16)> WithOptionValue, std::vector<std::pair<uint16, Value>>& vecValue)
{
    uint16 unNum;
    if (DecodeShort(pBuff, unNum))
    {
        vecValue.clear();
        uint16 unId;
        Value stValue;
        for (uint16 i = 0; i < unNum; ++i)
        {
            stValue.Clear();
            if (!DecodeOption(pBuff, WithOptionValue, unId, stValue))
            {
                return(false);
            }
            vecValue.push_back(std::move(std::make_pair(unId, std::move(stValue))));
        }
        return(true);
    }
    return(false);
}

bool Notations::DecodeInet(CBuffer* pBuff, InetAddr& stAddr, int& iPort)
{
    return(DecodeInetaddr(pBuff, stAddr) && DecodeInt(pBuff, iPort));
}

bool Notations::DecodeInetaddr(CBuffer* pBuff, InetAddr& stAddr)
{
    char cLen = 0;
    if (!pBuff->ReadByte(cLen))
    {
        return(false);
    }
    if (cLen == 4)  // ipv4
    {
        stAddr.type = cLen;
        if (pBuff->ReadableBytes() < 4)
        {
            return(false);
        }
        pBuff->Read(&stAddr.addr[0], 4);
        stAddr.addr[0] = CodecUtil::N2H(stAddr.addr[0]);
        return(true);
    }
    else if (cLen == 16) // ipv6
    {
        stAddr.type = cLen;
        if (pBuff->ReadableBytes() < 16)
        {
            return(false);
        }
        pBuff->Read(&stAddr.addr[0], 4);
        stAddr.addr[0] = CodecUtil::N2H(stAddr.addr[0]);
        pBuff->Read(&stAddr.addr[1], 4);
        stAddr.addr[1] = CodecUtil::N2H(stAddr.addr[1]);
        pBuff->Read(&stAddr.addr[2], 4);
        stAddr.addr[2] = CodecUtil::N2H(stAddr.addr[2]);
        pBuff->Read(&stAddr.addr[3], 4);
        stAddr.addr[3] = CodecUtil::N2H(stAddr.addr[3]);
        return(true);
    }
    else
    {
        return(false);
    }
}

bool Notations::DecodeConsistency(CBuffer* pBuff, uint16& unValue)
{
    return(DecodeShort(pBuff, unValue));
}

bool Notations::DecodeStringMap(CBuffer* pBuff, std::unordered_map<std::string, std::string>& mapValue)
{
    uint16 unNum;
    if (DecodeShort(pBuff, unNum))
    {
        std::string strKey;
        std::string strValue;
        for (uint16 i = 0; i < unNum; ++i)
        {
            if (!DecodeString(pBuff, strKey) || !DecodeString(pBuff, strValue))
            {
                return(false);
            }
            mapValue.insert(std::make_pair(strKey, strValue));
        }
        return(true);
    }
    return(false);
}

bool Notations::DecodeStringMultimap(CBuffer* pBuff, std::unordered_map<std::string, std::vector<std::string>>& mapValue)
{
    uint16 unNum;
    if (DecodeShort(pBuff, unNum))
    {
        std::string strKey;
        std::vector<std::string> vecValue;
        for (uint16 i = 0; i < unNum; ++i)
        {
            vecValue.clear();
            if (!DecodeString(pBuff, strKey) || !DecodeStringList(pBuff, vecValue))
            {
                return(false);
            }
            mapValue.insert(std::move(std::make_pair(strKey, std::move(vecValue))));
        }
        return(true);
    }
    return(false);
}

bool Notations::DecodeBytesMap(CBuffer* pBuff, std::unordered_map<std::string, Bytes>& mapValue)
{
    uint16 unNum;
    if (DecodeShort(pBuff, unNum))
    {
        std::string strKey;
        Bytes stValue;
        for (uint16 i = 0; i < unNum; ++i)
        {
            if (!DecodeString(pBuff, strKey) || !DecodeBytes(pBuff, stValue))
            {
                return(false);
            }
            mapValue.insert(std::make_pair(strKey, std::move(stValue)));
        }
        return(true);
    }
    return(false);
}

} /* namespace neb */

