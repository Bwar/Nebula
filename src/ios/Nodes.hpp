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
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include "Definition.hpp"
#include "channel/Channel.hpp"

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
    HASH_cityhash_32        = 3,
};

/**
 * @brief 节点管理
 */
class Nodes
{
public:
    /**
     * @note 节点管理Session构造函数
     * @param iHashAlgorithm hash算法
     * @param iVirtualNodeNum 每个实体节点对应的虚拟节点数量
     */
    Nodes(int iHashAlgorithm = HASH_murmur3_32, int iVirtualNodeNum = 200);
    virtual ~Nodes();

    /* 实体节点hash信息
     * key为Identify字符串
     * value为hash(Property001#0) hash(Property001#1) hash(Property001#2) 组成的vector */
    typedef std::unordered_map<std::string, std::vector<uint32> > T_NODE2HASH_MAP;

    struct tagNode
    {
        bool bCheckFailedNode = false;
        std::string strNodeType;
        T_NODE2HASH_MAP mapNode2Hash;
        T_NODE2HASH_MAP::iterator itPollingNode;
        std::map<uint32, std::string> mapHash2Node;
        std::map<uint32, std::string>::const_iterator itHashRing;
        std::set<std::string> setFailedNode;
        std::set<std::string>::const_iterator itPollingFailed;

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

    bool GetNodeInHashRing(const std::string& strNodeType, std::string& strNodeIdentify);

    bool GetNode(const std::string& strNodeType, std::string& strNodeIdentify);

    bool GetNode(const std::string& strNodeType, std::set<std::string>& setNodeIdentify);

    bool NodeDetect(const std::string& strNodeType, std::string& strNodeIdentify);

    /**
     * @brief 添加节点
     * @note 添加节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeType 节点类型
     * @param strNodeIdentify 节点标识
     */
    void AddNode(const std::string& strNodeType, const std::string& strNodeIdentify);

    /** @deprecate */
    void AddNodeKetama(const std::string& strNodeType, const std::string& strNodeIdentify);

    void ReplaceNodes(const std::string& strNodeType, const std::set<std::string>& setNodeIdentify);

    /**
     * @brief 添加并获取节点
     * @note 将形如192.168.1.47:6379,192.168.1.53:6379,192.168.1.38:6379的
     * strNodeType分割后添加节点信息并且获取其中一个节点。
     * @param[in] strNodeType 节点类型
     * @param[out] strNodeIdentify 节点标识
     */
    bool SplitAddAndGetNode(const std::string& strNodeType, std::string& strNodeIdentify);

    bool GetAuth(const std::string& strIdentify, std::string& strAuth, std::string& strPassword);

    std::shared_ptr<ChannelOption> GetChannelOption(const std::string& strIdentify);

    void SetChannelOption(const std::string& strIdentify, const ChannelOption& stOption);
    void SetDefaultChannelOption(const ChannelOption& stOption);

    /**
     * @brief 删除节点
     * @note 删除节点信息，每个节点均有一个主节点一个被节点构成。
     * @param strNodeType 节点类型
     * @param strNodeIdentify 节点标识
     */
    void DelNode(const std::string& strNodeType, const std::string& strNodeIdentify);

    /**
     * @brief 节点失败
     * @not 节点失败达到一定数量会熔断，待探测成功后会重新加入服务节点列表。
     * @param strNodeIdentify 节点标识
     */
    void NodeFailed(const std::string& strNodeIdentify);

    /**
     * @brief 节点恢复
     * @param strNodeIdentify 节点标识
     */
    void NodeRecover(const std::string& strNodeIdentify);

    bool IsNodeType(const std::string& strNodeIdentify, const std::string& strNodeType);

    void CheckFailedNode();

public:
    static uint32 hash_fnv1_64(const char *key, size_t key_length);
    static uint32 hash_fnv1a_64(const char *key, size_t key_length);
    static uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed);

private:
    const int m_iHashAlgorithm;
    const int m_iVirtualNodeNum;
    std::shared_ptr<ChannelOption> m_pDefaultChannelOption;
    std::unordered_map<std::string, std::shared_ptr<tagNode> > m_mapNode;
    std::unordered_map<std::string, std::set<std::string>> m_mapNodeType;  // key为节点标识
    std::unordered_map<std::string, std::shared_ptr<ChannelOption>> m_mapChannelOption;  // key为节点标识
};

} /* namespace neb */

#endif /* SRC_IOS_NODES_HPP_ */
