
/*******************************************************************************
 * Project:  Nebula
 * @file     Nodes.cpp
 * @brief 
 * @author   bwar
 * @date:    2016-3-19
 * @note
 * Modify history:
 ******************************************************************************/
#include "Nodes.hpp"
#include <cstring>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/md5.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "util/encrypt/city.h"
#include "util/StringCoder.hpp"

namespace neb
{

Nodes::Nodes(int iHashAlgorithm, int iVirtualNodeNum)
    : m_iHashAlgorithm(iHashAlgorithm), m_iVirtualNodeNum(iVirtualNodeNum)
{
}

Nodes::~Nodes()
{
    m_mapNode.clear();
    m_mapNodeType.clear();
}

bool Nodes::GetNode(const std::string& strNodeType, const std::string& strHashKey, std::string& strNodeIdentify)
{
    uint32 uiKeyHash = 0;
    switch (m_iHashAlgorithm)
    {
        case HASH_cityhash_32:
            uiKeyHash = CityHash32(strHashKey.c_str(), strHashKey.size());
            break;
        case HASH_fnv1_64:
            uiKeyHash = hash_fnv1_64(strHashKey.c_str(), strHashKey.size());
            break;
        case HASH_murmur3_32:
            uiKeyHash = murmur3_32(strHashKey.c_str(), strHashKey.size(), 0x000001b3);
            break;
        default:
            uiKeyHash = hash_fnv1a_64(strHashKey.c_str(), strHashKey.size());
    }

    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        if (node_type_iter->second->mapHash2Node.size() == 0)
        {
            if (node_type_iter->second->setFailedNode.size() > 0)
            {
                if (node_type_iter->second->itPollingFailed == node_type_iter->second->setFailedNode.end())
                {
                    node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
                }
                strNodeIdentify = *(node_type_iter->second->itPollingFailed);
                node_type_iter->second->itPollingFailed++;
                return(true);
            }
            return(false);
        }
        auto c_iter = node_type_iter->second->mapHash2Node.lower_bound(uiKeyHash);
        if (c_iter == node_type_iter->second->mapHash2Node.end())
        {
            strNodeIdentify = node_type_iter->second->mapHash2Node.begin()->second;
        }
        else
        {
            strNodeIdentify = c_iter->second;
            node_type_iter->second->itHashRing = c_iter;
        }
        return(true);
    }
}

bool Nodes::GetNode(const std::string& strNodeType, uint32 uiHash, std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        if (node_type_iter->second->mapHash2Node.size() == 0)
        {
            if (node_type_iter->second->setFailedNode.size() > 0)
            {
                if (node_type_iter->second->itPollingFailed == node_type_iter->second->setFailedNode.end())
                {
                    node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
                }
                strNodeIdentify = *(node_type_iter->second->itPollingFailed);
                node_type_iter->second->itPollingFailed++;
                return(true);
            }
            return(false);
        }
        auto c_iter = node_type_iter->second->mapHash2Node.lower_bound(uiHash);
        if (c_iter == node_type_iter->second->mapHash2Node.end())
        {
            strNodeIdentify = node_type_iter->second->mapHash2Node.begin()->second;
        }
        else
        {
            strNodeIdentify = c_iter->second;
            node_type_iter->second->itHashRing = c_iter;
        }
        return(true);
    }
}

bool Nodes::GetNodeInHashRing(const std::string& strNodeType, std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        if (node_type_iter->second->mapHash2Node.size() == 0)
        {
            return(false);
        }
        if (node_type_iter->second->itHashRing == node_type_iter->second->mapHash2Node.end())
        {
            node_type_iter->second->itHashRing = node_type_iter->second->mapHash2Node.begin();
        }
        strNodeIdentify = node_type_iter->second->itHashRing->first;
        node_type_iter->second->itHashRing++;
        return(true);
    }
}

bool Nodes::GetNode(const std::string& strNodeType, std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        if (node_type_iter->second->mapHash2Node.size() == 0)
        {
            if (node_type_iter->second->setFailedNode.size() > 0)
            {
                if (node_type_iter->second->itPollingFailed == node_type_iter->second->setFailedNode.end())
                {
                    node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
                }
                strNodeIdentify = *(node_type_iter->second->itPollingFailed);
                node_type_iter->second->itPollingFailed++;
                return(true);
            }
            return(false);
        }
        if (node_type_iter->second->itPollingNode == node_type_iter->second->mapNode2Hash.end())
        {
            node_type_iter->second->itPollingNode = node_type_iter->second->mapNode2Hash.begin();
        }
        strNodeIdentify = node_type_iter->second->itPollingNode->first;
        node_type_iter->second->itPollingNode++;
        return(true);
    }
}

bool Nodes::GetNode(const std::string& strNodeType, std::set<std::string>& setNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        for (auto iter = node_type_iter->second->mapNode2Hash.begin(); iter != node_type_iter->second->mapNode2Hash.end(); ++iter)
        {
            setNodeIdentify.insert(iter->first);
        }
        return(true);
    }
}

bool Nodes::NodeDetect(const std::string& strNodeType, std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        return(false);
    }
    else
    {
        if (node_type_iter->second->bCheckFailedNode)
        {
            node_type_iter->second->bCheckFailedNode = false;
            if (node_type_iter->second->setFailedNode.size() > 0)
            {
                if (node_type_iter->second->itPollingFailed == node_type_iter->second->setFailedNode.end())
                {
                    node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
                }
                strNodeIdentify = *(node_type_iter->second->itPollingFailed);
                node_type_iter->second->itPollingFailed++;
                return(true);
            }
        }
        return(false);
    }
}

void Nodes::AddNode(const std::string& strNodeType, const std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        std::shared_ptr<tagNode> pNode = std::make_shared<tagNode>();
        m_mapNode.insert(std::make_pair(strNodeType, pNode));
        pNode->strNodeType = strNodeType;
        std::string strHashKey;
        char szVirtualNodeIdentify[40] = {0};
        uint32 uiKeyHash = 0;
        std::vector<uint32> vecHash;
        for (int i = 0; i < m_iVirtualNodeNum; ++i)
        {
            snprintf(szVirtualNodeIdentify, 40, "%d@%s#%d", m_iVirtualNodeNum - i, strNodeIdentify.c_str(), i);
            strHashKey = szVirtualNodeIdentify;
            switch (m_iHashAlgorithm)
            {
                case HASH_cityhash_32:
                    uiKeyHash = CityHash32(strHashKey.c_str(), strHashKey.size());
                    break;
                case HASH_fnv1_64:
                    uiKeyHash = hash_fnv1_64(strHashKey.c_str(), strHashKey.size());
                    break;
                case HASH_murmur3_32:
                    uiKeyHash = murmur3_32(strHashKey.c_str(), strHashKey.size(), 0x000001b3);
                    break;
                default:
                    uiKeyHash = hash_fnv1a_64(strHashKey.c_str(), strHashKey.size());
            }
            vecHash.push_back(uiKeyHash);
            pNode->mapHash2Node.insert(std::make_pair(uiKeyHash, strNodeIdentify));
        }
        pNode->mapNode2Hash.insert(std::make_pair(strNodeIdentify, std::move(vecHash)));
        pNode->itPollingNode = pNode->mapNode2Hash.begin();
        pNode->itHashRing = pNode->mapHash2Node.begin();
    }
    else
    {
        auto node_iter = node_type_iter->second->mapNode2Hash.find(strNodeIdentify);
        if (node_iter == node_type_iter->second->mapNode2Hash.end())
        {
            std::string strHashKey;
            char szVirtualNodeIdentify[40] = {0};
            uint32 uiKeyHash = 0;
            std::vector<uint32> vecHash;
            for (int i = 0; i < m_iVirtualNodeNum; ++i)
            {
                snprintf(szVirtualNodeIdentify, 40, "%d@%s#%d", m_iVirtualNodeNum - i, strNodeIdentify.c_str(), i);
                strHashKey = szVirtualNodeIdentify;
                switch (m_iHashAlgorithm)
                {
                    case HASH_cityhash_32:
                        uiKeyHash = CityHash32(strHashKey.c_str(), strHashKey.size());
                        break;
                    case HASH_fnv1_64:
                        uiKeyHash = hash_fnv1_64(strHashKey.c_str(), strHashKey.size());
                        break;
                    case HASH_murmur3_32:
                        uiKeyHash = murmur3_32(strHashKey.c_str(), strHashKey.size(), 0x000001b3);
                        break;
                    default:
                        uiKeyHash = hash_fnv1a_64(strHashKey.c_str(), strHashKey.size());
                }
                vecHash.push_back(uiKeyHash);
                node_type_iter->second->mapHash2Node.insert(std::make_pair(uiKeyHash, strNodeIdentify));
            }
            node_type_iter->second->mapNode2Hash.insert(std::make_pair(strNodeIdentify, std::move(vecHash)));
            node_type_iter->second->itPollingNode = node_type_iter->second->mapNode2Hash.begin();
            node_type_iter->second->itHashRing = node_type_iter->second->mapHash2Node.begin();
        }
    }
    auto node_id_iter = m_mapNodeType.find(strNodeIdentify);
    if (node_id_iter == m_mapNodeType.end())
    {
        std::set<std::string> setNodeType;
        setNodeType.insert(strNodeType);
        m_mapNodeType.insert(std::make_pair(strNodeIdentify, std::move(setNodeType)));
    }
    else
    {
        node_id_iter->second.insert(strNodeType);
    }
}

void Nodes::AddNodeKetama(const std::string& strNodeType, const std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        std::shared_ptr<tagNode> pNode = std::make_shared<tagNode>();
        m_mapNode.insert(std::make_pair(strNodeType, pNode));
        pNode->strNodeType = strNodeType;
        std::string strHash;
        char szVirtualNodeIdentify[40] = {0};
        int32 iPointPerHash = 4;
        std::vector<uint32> vecHash;
        for (int i = 0; i < m_iVirtualNodeNum / iPointPerHash; ++i)     // distribution: ketama
        {
            snprintf(szVirtualNodeIdentify, 40, "%d@%s#%d", m_iVirtualNodeNum - i, strNodeIdentify.c_str(), i);
            CryptoPP::Weak1::MD5 oMd5;
            CryptoPP::HexEncoder oHexEncoder;
            oMd5.Update((const CryptoPP::byte*)szVirtualNodeIdentify, strlen(szVirtualNodeIdentify));
            strHash.resize(oMd5.DigestSize());
            oMd5.Final((CryptoPP::byte*)&strHash[0]);
            for (int j = 0; j < iPointPerHash; ++j)
            {
                uint32 k = ((uint32)(strHash[3 + j * iPointPerHash] & 0xFF) << 24)
                       | ((uint32)(strHash[2 + j * iPointPerHash] & 0xFF) << 16)
                       | ((uint32)(strHash[1 + j * iPointPerHash] & 0xFF) << 8)
                       | (strHash[j * iPointPerHash] & 0xFF);
                vecHash.push_back(k);
                pNode->mapHash2Node.insert(std::make_pair(k, strNodeIdentify));
            }
        }
        pNode->mapNode2Hash.insert(std::make_pair(strNodeIdentify, std::move(vecHash)));
        pNode->itPollingNode = pNode->mapNode2Hash.begin();
        pNode->itHashRing = pNode->mapHash2Node.begin();
    }
    else
    {
        auto node_iter = node_type_iter->second->mapNode2Hash.find(strNodeIdentify);
        if (node_iter == node_type_iter->second->mapNode2Hash.end())
        {
            std::string strHash;
            char szVirtualNodeIdentify[40] = {0};
            int32 iPointPerHash = 4;
            std::vector<uint32> vecHash;
            for (int i = 0; i < m_iVirtualNodeNum / iPointPerHash; ++i)     // distribution: ketama
            {
                snprintf(szVirtualNodeIdentify, 40, "%d@%s#%d", m_iVirtualNodeNum - i, strNodeIdentify.c_str(), i);
                CryptoPP::Weak1::MD5 oMd5;
                CryptoPP::HexEncoder oHexEncoder;
                oMd5.Update((const CryptoPP::byte*)szVirtualNodeIdentify, strlen(szVirtualNodeIdentify));
                strHash.resize(oMd5.DigestSize());
                oMd5.Final((CryptoPP::byte*)&strHash[0]);
                for (int j = 0; j < iPointPerHash; ++j)
                {
                    uint32 k = ((uint32)(strHash[3 + j * iPointPerHash] & 0xFF) << 24)
                           | ((uint32)(strHash[2 + j * iPointPerHash] & 0xFF) << 16)
                           | ((uint32)(strHash[1 + j * iPointPerHash] & 0xFF) << 8)
                           | (strHash[j * iPointPerHash] & 0xFF);
                    vecHash.push_back(k);
                    node_type_iter->second->mapHash2Node.insert(std::make_pair(k, strNodeIdentify));
                }
            }
            node_type_iter->second->mapNode2Hash.insert(std::make_pair(strNodeIdentify, std::move(vecHash)));
            node_type_iter->second->itPollingNode = node_type_iter->second->mapNode2Hash.begin();
            node_type_iter->second->itHashRing = node_type_iter->second->mapHash2Node.begin();
        }
    }
    auto node_id_iter = m_mapNodeType.find(strNodeIdentify);
    if (node_id_iter == m_mapNodeType.end())
    {
        std::set<std::string> setNodeType;
        setNodeType.insert(strNodeType);
        m_mapNodeType.insert(std::make_pair(strNodeIdentify, std::move(setNodeType)));
    }
    else
    {
        node_id_iter->second.insert(strNodeType);
    }
}

void Nodes::ReplaceNodes(const std::string& strNodeType, const std::set<std::string>& setNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter == m_mapNode.end())
    {
        for (auto it = setNodeIdentify.begin(); it != setNodeIdentify.end(); ++it)
        {
            AddNode(strNodeType, *it);
        }
    }
    else
    {
        m_mapNode.erase(node_type_iter);
        for (auto it = setNodeIdentify.begin(); it != setNodeIdentify.end(); ++it)
        {
            AddNode(strNodeType, *it);
        }
    }
}

bool Nodes::SplitAddAndGetNode(const std::string& strNodeType, std::string& strNodeIdentify)
{
    std::vector<std::string> vecAddress;
    Split(strNodeType, ",", vecAddress);
    if (vecAddress.size() < 1)
    {
        return(false);
    }
    for (size_t i = 0; i < vecAddress.size(); ++i)
    {
        if (vecAddress[i].size() < 5)
        {
            continue;
        }
        AddNode(strNodeType, vecAddress[i]);
    }
    return(GetNode(strNodeType, strNodeIdentify));
}

bool Nodes::GetAuth(const std::string& strIdentify, std::string& strAuth, std::string& strPassword)
{
    auto iter = m_mapChannelOption.find(strIdentify);
    if (iter == m_mapChannelOption.end())
    {
        return(false);
    }
    else
    {
        strAuth = iter->second->strAuth;
        strPassword = iter->second->strPassword;
        return(true);
    }
}

std::shared_ptr<ChannelOption> Nodes::GetChannelOption(const std::string& strIdentify)
{
    auto iter = m_mapChannelOption.find(strIdentify);
    if (iter == m_mapChannelOption.end())
    {
        return(nullptr);
    }
    else
    {
        return(iter->second);
    }
}

void Nodes::SetChannelOption(const std::string& strIdentify, const ChannelOption& stOption)
{
    auto pOption = std::make_shared<ChannelOption>(stOption);
    auto iter = m_mapChannelOption.find(strIdentify);
    if (iter == m_mapChannelOption.end())
    {
        m_mapChannelOption.insert(std::make_pair(strIdentify, pOption));
    }
    else
    {
        iter->second = pOption;
    }
}

void Nodes::DelNode(const std::string& strNodeType, const std::string& strNodeIdentify)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter != m_mapNode.end())
    {
        auto node_iter = node_type_iter->second->mapNode2Hash.find(strNodeIdentify);
        if (node_iter != node_type_iter->second->mapNode2Hash.end())
        {
            for (std::vector<uint32>::iterator hash_iter = node_iter->second.begin();
                            hash_iter != node_iter->second.end(); ++hash_iter)
            {
                auto it = node_type_iter->second->mapHash2Node.find(*hash_iter);
                if (it != node_type_iter->second->mapHash2Node.end())
                {
                    node_type_iter->second->mapHash2Node.erase(it);
                }
            }
            node_type_iter->second->mapNode2Hash.erase(node_iter);
            node_type_iter->second->itPollingNode = node_type_iter->second->mapNode2Hash.begin();
            node_type_iter->second->itHashRing = node_type_iter->second->mapHash2Node.begin();
        }

        if (node_type_iter->second->setFailedNode.size() > 0)
        {
            auto it = node_type_iter->second->setFailedNode.find(strNodeIdentify);
            if (it != node_type_iter->second->setFailedNode.end())
            {
                node_type_iter->second->setFailedNode.erase(it);
                node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
            }
        }

        if (node_type_iter->second->mapNode2Hash.empty())
        {
            m_mapNode.erase(node_type_iter);
        }
    }
    auto node_id_iter = m_mapNodeType.find(strNodeIdentify);
    if (node_id_iter != m_mapNodeType.end())
    {
        m_mapNodeType.erase(node_id_iter);
    }
}

void Nodes::NodeFailed(const std::string& strNodeIdentify)
{
    auto node_id_iter = m_mapNodeType.find(strNodeIdentify);
    if (node_id_iter == m_mapNodeType.end())
    {
        ;   // not found
    }
    else
    {
        for (auto type_it = node_id_iter->second.begin(); type_it != node_id_iter->second.end(); ++type_it)
        {
            auto node_type_iter = m_mapNode.find(*type_it);
            if (node_type_iter != m_mapNode.end())
            {
                if (node_type_iter->second->mapNode2Hash.size() <= 1)
                {
                    continue;  // only one node, should not be circuit break.
                }
                auto node_iter = node_type_iter->second->mapNode2Hash.find(strNodeIdentify);
                if (node_iter != node_type_iter->second->mapNode2Hash.end())
                {
                    for (auto hash_iter = node_iter->second.begin();
                            hash_iter != node_iter->second.end(); ++hash_iter)
                    {
                        auto it = node_type_iter->second->mapHash2Node.find(*hash_iter);
                        if (it != node_type_iter->second->mapHash2Node.end())
                        {
                            node_type_iter->second->mapHash2Node.erase(it);
                        }
                    }
                    node_type_iter->second->mapNode2Hash.erase(node_iter);
                    node_type_iter->second->itPollingNode = node_type_iter->second->mapNode2Hash.begin();
                    node_type_iter->second->itHashRing = node_type_iter->second->mapHash2Node.begin();
                }
                node_type_iter->second->setFailedNode.insert(strNodeIdentify);
                node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
            }
        }
    }
}

void Nodes::NodeRecover(const std::string& strNodeIdentify)
{
    auto node_id_iter = m_mapNodeType.find(strNodeIdentify);
    if (node_id_iter == m_mapNodeType.end())
    {
        ;   // not found
    }
    else
    {
        for (auto type_it = node_id_iter->second.begin(); type_it != node_id_iter->second.end(); ++type_it)
        {
            auto node_type_iter = m_mapNode.find(*type_it);
            if (node_type_iter != m_mapNode.end())
            {
                if (node_type_iter->second->setFailedNode.size() > 0)
                {
                    auto it = node_type_iter->second->setFailedNode.find(strNodeIdentify);
                    if (it != node_type_iter->second->setFailedNode.end())
                    {
                        node_type_iter->second->setFailedNode.erase(it);
                        node_type_iter->second->itPollingFailed = node_type_iter->second->setFailedNode.begin();
                    }
                }
                AddNode(*type_it, strNodeIdentify);
            }
        }
    }
}

bool Nodes::IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType)
{
    auto node_type_iter = m_mapNode.find(strNodeType);
    if (node_type_iter != m_mapNode.end())
    {
        auto node_iter = node_type_iter->second->mapNode2Hash.find(strNodeIdentify);
        if (node_iter != node_type_iter->second->mapNode2Hash.end())
        {
            return(true);
        }
    }
    return(false);
}

void Nodes::CheckFailedNode()
{
    for (auto iter = m_mapNode.begin(); iter != m_mapNode.end(); ++iter)
    {
        iter->second->bCheckFailedNode = true;
    }
}

uint32 Nodes::hash_fnv1_64(const char *key, size_t key_length)
{
    uint64_t hash = FNV_64_INIT;
    size_t x;

    for (x = 0; x < key_length; x++) {
      hash *= FNV_64_PRIME;
      hash ^= (uint64_t)key[x];
    }

    return (uint32_t)hash;
}

uint32 Nodes::hash_fnv1a_64(const char *key, size_t key_length)
{
    uint32_t hash = (uint32_t) FNV_64_INIT;
    size_t x;

    for (x = 0; x < key_length; x++) {
      uint32_t val = (uint32_t)key[x];
      hash ^= val;
      hash *= (uint32_t) FNV_64_PRIME;
    }

    return hash;
}

uint32_t Nodes::murmur3_32(const char *key, uint32_t len, uint32_t seed)
{
    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m = 5;
    static const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;
    auto ROT32 = [](uint32_t x, uint32_t y){return((x << y) | (x >> (32 - y)));};

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *) key;
    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++)
    {
        k = blocks[i];
        k *= c1;
        k = ROT32(k, r1);
        k *= c2;

        hash ^= k;
        hash = ROT32(hash, r2) * m + n;
    }

    const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];

        k1 *= c1;
        k1 = ROT32(k1, r1);
        k1 *= c2;
        hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

} /* namespace neb */
