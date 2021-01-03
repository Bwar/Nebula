/*******************************************************************************
 * Project:  Nebula
 * @file     StepRedisCluster.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-12-12
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_ACTOR_STEP_SYS_STEP_STEPREDISCLUSTER_HPP_
#define SRC_ACTOR_STEP_SYS_STEP_STEPREDISCLUSTER_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "actor/step/RedisStep.hpp"
#include "actor/ActorSys.hpp"

#define REDIS_COMMAND_ASKING "ASKING"
#define REDIS_COMMAND_PING "PING"

namespace neb
{

class Dispatcher;

enum E_REDIS_CMD_TYPE
{
    REDIS_CMD_READ  = 0,
    REDIS_CMD_WRITE = 1,
};

struct RedisNode
{
    std::string strMaster;
    std::unordered_set<std::string> setFllower;
    std::unordered_set<std::string>::iterator iterFllower;
    RedisNode(){}
    RedisNode(const RedisNode& stNode) = delete;
    RedisNode(RedisNode&& stNode)
    {
        strMaster = std::move(stNode.strMaster);
        setFllower = std::move(stNode.setFllower);
        iterFllower = setFllower.begin();
    }
    RedisNode& operator=(const RedisNode& stNode) = delete;
    RedisNode& operator=(RedisNode&& stNode)
    {
        strMaster = std::move(stNode.strMaster);
        setFllower = std::move(stNode.setFllower);
        iterFllower = setFllower.begin();
        return(*this);
    }
};

class StepRedisCluster: public RedisStep,
    public DynamicCreator<StepRedisCluster, const std::string&, bool, bool, bool>,
    public ActorSys
{
public:
    StepRedisCluster(const std::string& strIdentify, bool bWithSsl, bool bPipeline, bool bEnableReadOnly);
    virtual ~StepRedisCluster();

    virtual E_CMD_STATUS Emit(int iErrno = neb::ERR_OK, const std::string& strErrMsg = "", void* data = NULL);

    virtual E_CMD_STATUS Callback(
            std::shared_ptr<SocketChannel> pChannel,
            const RedisReply& oRedisReply);

    virtual E_CMD_STATUS Timeout();

    virtual E_CMD_STATUS ErrBack(
            std::shared_ptr<SocketChannel> pChannel,
            int iErrno, const std::string& strErrMsg) override;


    virtual bool SendTo(const std::string& strIdentify, const RedisMsg& oRedisMsg,
            bool bWithSsl, bool bPipeline, uint32 uiStepSeq) override;

protected:
    bool SendTo(const std::string& strIdentify, std::shared_ptr<RedisMsg> pRedisMsg);
    bool ExtractCmd(const RedisMsg& oRedisMsg, std::string& strCmd,
            std::vector<std::string>& vecHashKeys, int& iReadOrWrite, int& iKeyInterval);
    bool GetRedisNode(int iSlotId, int iReadOrWrite, std::string& strNodeIdentify, bool& bIsMaster) const;
    bool NeedSetReadOnly(const std::string& strNode) const;
    bool SendCmdClusterSlots();
    bool CmdClusterSlotsCallback(const RedisReply& oRedisReply);
    bool SendCmdAsking(const std::string& strIdentify);
    void CmdAskingCallback(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const RedisReply& oRedisReply);
    bool SendCmdReadOnly(const std::string& strIdentify);
    void CmdReadOnlyCallback(std::shared_ptr<SocketChannel> pChannel, const std::string& strIdentify, const RedisReply& oRedisReply);
    void AskingQueueErrBack(std::shared_ptr<SocketChannel> pChannel,
            int iErrno, const std::string& strErrMsg);
    bool Dispatch(const RedisMsg& oRedisMsg, uint32 uiStepSeq);
    void SendWaittingRequest();
    void RegisterStep(uint32 uiStepSeq);
    void AddToAskingQueue(const std::string& strIdentify, std::shared_ptr<RedisMsg> pRedisMsg);

private:
    bool m_bWithSsl;                        ///< 是否支持SSL
    bool m_bPipeline;                       ///< 是否支持pipeline
    bool m_bEnableReadOnly;                 ///< 是否对从节点启用只读
    uint32 m_uiAddressIndex;

    std::string m_strIdentify;      ///< 地址标识，由m_vecAddress合并而成，形如 192.168.47.101:6379,198.168.47.102:6379,192.168.47.103:6379
    std::vector<std::string> m_vecAddress;  ///< 集群地址
    std::unordered_map<int, std::shared_ptr<RedisNode>> m_mapSlot2Node;    // redis cluster slot对应的节点
    std::unordered_map<std::string, std::queue<std::shared_ptr<RedisRequest>>> m_mapPipelineRequest;  ///< 等待回调的请求
    std::unordered_map<std::string, std::queue<std::shared_ptr<RedisRequest>>> m_mapAskingRequest;  ///< 等待Asking的请求
    std::unordered_map<uint32, uint32> m_mapStepEmitNum;    // 每个step发出请求（等待响应）数量
    std::unordered_map<uint32, std::vector<RedisReply*>> m_mapReply;
    std::map<time_t, std::vector<uint32>> m_mapTimeoutStep;
    std::vector<std::pair<uint32, RedisRequest>> m_vecWaittingRequest;

    static const uint16 sc_unClusterSlots;  ///< redis cluster槽位数
    static const std::unordered_set<std::string> s_setSupportExtractCmd;
    static const std::unordered_set<std::string> s_setReadCmd;
    static const std::unordered_set<std::string> s_setWriteCmd;
    static const std::unordered_set<std::string> s_setMultipleKeyCmd;
    static const std::unordered_set<std::string> s_setMultipleKeyValueCmd;
};

} /* namespace neb */

#endif /* SRC_ACTOR_STEP_SYS_STEP_STEPREDISCLUSTER_HPP_ */

