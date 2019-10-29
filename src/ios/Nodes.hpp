/*******************************************************************************
 * Project:  Nebula
 * @file     Nodes.hpp
 * @brief    节点Session
 * @author   bwar
 * @date:    2016年3月19日
 * @note     存储节点信息，提供节点的添加、删除、修改操作，提供通过
 * hash字符串或hash值定位具体节点操作。
 * Modify history:
 ******************************************************************************/
#ifndef SRC_IOS_NODES_HPP_
#define SRC_IOS_NODES_HPP_

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "Definition.hpp"

//#define ROT32(x, y) ((x << y) | (x >> (32 - y))) // avoid effort

namespace neb
{

const unsigned long FNV_64_INIT = 0x100000001b3;
const unsigned long FNV_64_PRIME = 0xcbf29ce484222325;

enum E_HASH_ALGORITHM
{
    HASH_fnv1a_64           = 0,
    HASH_fnv1_64            = 1,
    HASH_murmur3_32         = 2,
};

/**
 * @brief 节点管理
 */
class Nodes
{
public:
    /**
     * @note 节点管理Session构造函数
     * @param iVirtualNodeNum 每个实体节点对应的虚拟节点数量
     * @param dSessionTimeout 超时时间，0表示永不超时
     */
    Nodes(int iHashAlgorithm = HASH_fnv1a_64, int iVirtualNodeNum = 200);
    virtual ~Nodes();

    /* 实体节点hash信息
     * key为Identify字符串
     * value为hash(Property001#0) hash(Property001#1) hash(Property001#2) 组成的vector */
    typedef std::unordered_map<std::string, std::vector<uint32> > T_NODE2HASH_MAP;

    struct tagNode
    {
        T_NODE2HASH_MAP mapNode2Hash;
        T_NODE2HASH_MAP::iterator itPollingNode;
        std::map<uint32, std::string> mapHash2Node;

        tagNode(){}
        ~tagNode(){}
        tagNode(const tagNode& stNode) = delete;
        tagNode& operator=(const tagNode& stNode) = delete;
    };

public:
    /**
     * @brief 获取节点信息
     * @note 通过hash key获取一致性hash算法计算后对应的主备节点信息
     * @param[in] strNodeType节点类型
     * @param[in] strHashKey 数据操作的key值
     * @param[out] strNodeIdentify 节点标识
     * @return bool 是否成功获取节点信息
     */
    bool GetNode(const std::string& strNodeType, const std::string& strHashKey, std::string& strNodeIdentify);

    bool GetNode(const std::string& strNodeType, uint32 uiHash, std::string& strNodeIdentify);

    bool GetNode(const std::string& strNodeType, std::string& strNodeIdentify);

    bool GetNode(const std::string& strNodeType, std::unordered_set<std::string>& setNodeIdentify);

    /**
     * @brief 添加节点
     * @note 添加节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeIdentify 节点标识
     */
    void AddNode(const std::string& strNodeType, const std::string& strNodeIdentify);

    /**
     * @brief 删除节点
     * @note 删除节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeIdentify 节点标识
     */
    void DelNode(const std::string& strNodeType, const std::string& strNodeIdentify);

    bool IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType);

protected:
    uint32 hash_fnv1_64(const char *key, size_t key_length);
    uint32 hash_fnv1a_64(const char *key, size_t key_length);
    uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed);

private:
    const int m_iHashAlgorithm;
    const int m_iVirtualNodeNum;

    std::unordered_map<std::string,  std::shared_ptr<tagNode> > m_mapNode;
};

} /* namespace neb */

#endif /* SRC_IOS_NODES_HPP_ */
