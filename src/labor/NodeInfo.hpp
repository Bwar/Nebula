/*******************************************************************************
 * Project:  Nebula
 * @file     NodeInfo.hpp
 * @brief
 * @author   Bwar
 * @date:    2019年9月1日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef SRC_LABOR_NODEINFO_HPP_
#define SRC_LABOR_NODEINFO_HPP_

#include <string>
#include "Definition.hpp"
#include "codec/Codec.hpp"

namespace neb
{

struct NodeInfo
{
    NodeInfo(){}
    NodeInfo(const NodeInfo& stAttr) = delete;
    NodeInfo& operator=(const NodeInfo& stAttr) = delete;
    E_CODEC_TYPE eCodec             = CODEC_UNKNOW; ///< 接入端编解码器
    uint32 uiNodeId                 = 0;            ///< 节点ID（由beacon分配）
    uint32 uiWorkerNum              = 0;            ///< Worker子进程数量
    int32 iAddrPermitNum            = 0;            ///< IP地址统计时间内允许连接次数
    int32 iMsgPermitNum             = 0;            ///< 客户端统计时间内允许发送消息数量
    int32 iPortForServer            = 0;            ///< Server间通信监听端口，对应 iS2SListenFd
    int32 iPortForClient            = 0;            ///< 对Client通信监听端口，对应 iC2SListenFd
    int32 iGatewayPort              = 0;            ///< 对Client服务的真实端口
    bool bIsAccess                  = false;        ///< 是否接入Server
    ev_tstamp dIoTimeout            = 0.0;          ///< IO（连接）超时配置
    ev_tstamp dDataReportInterval   = 60.0;         ///< 统计数据上报时间间隔
    ev_tstamp dMsgStatInterval      = 0.0;          ///< 客户端连接发送数据包统计时间间隔
    ev_tstamp dAddrStatInterval     = 0.0;          ///< IP地址数据统计时间间隔
    ev_tstamp dStepTimeout          = 0.0;          ///< 步骤超时
    std::string strWorkPath;                        ///< 工作路径
    std::string strConfFile;                        ///< 配置文件
    std::string strNodeType;                        ///< 节点类型
    std::string strHostForServer;                   ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
    std::string strHostForClient;                   ///< 对Client服务的IP地址，对应 m_iC2SListenFd
    std::string strGateway;                         ///< 对Client服务的真实IP地址（此ip转发给m_strHostForClient）
    std::string strNodeIdentify;
};

/**
 * @brief 工作进程属性
 */
struct WorkerInfo
{
    int32 iWorkerPid        = 0;
    int iWorkerIndex        = 0;                    ///< 工作进程序号
    int iControlFd          = -1;                   ///< 与Manager进程通信的文件描述符（控制流）
    int iDataFd             = -1;                   ///< 与Manager进程通信的文件描述符（数据流）
    int32 iLoad             = 0;                    ///< 负载
    int32 iConnect          = 0;                    ///< 连接数量
    int32 iRecvNum          = 0;                    ///< 接收数据包数量
    int32 iRecvByte         = 0;                    ///< 接收字节数
    int32 iSendNum          = 0;                    ///< 发送数据包数量
    int32 iSendByte         = 0;                    ///< 发送字节数
    int32 iClientNum        = 0;                    ///< 客户端数量
    ev_tstamp dBeatTime     = 0.0;                  ///< 心跳时间
    bool bStartBeatCheck    = 0.0;                  ///< 是否需要心跳检查，worker或loader进程启动时可能需要加载数据而处于繁忙状态无法响应Manager的心跳，需等待其就绪之后才开始心跳检查。

    WorkerInfo(){}
    WorkerInfo(const WorkerInfo& stAttr) = delete;
    WorkerInfo& operator=(const WorkerInfo& stAttr) = delete;
};

} /* namespace neb */

#endif /* SRC_LABOR_NODEINFO_HPP_ */

