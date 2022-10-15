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
    uint32 uiLoaderNum              = 0;            ///< Loader子进程数量，有效值为0或1
    int32 iAddrPermitNum            = 0;            ///< IP地址统计时间内允许连接次数
    int32 iMsgPermitNum             = 0;            ///< 客户端统计时间内允许发送消息数量
    int32 iPortForServer            = 0;            ///< Server间通信监听端口，对应 iS2SListenFd
    int32 iPortForClient            = 0;            ///< 对Client通信监听端口，对应 iC2SListenFd
    int32 iForClientSocketType      = 0;            ///< 对Client通信的socket类型
    int32 iGatewayPort              = 0;            ///< 对Client服务的真实端口
    int32 iBacklog                  = 100;          ///< 监听队列长度
    int32 iConnectionDispatch       = 0;            ///< 新建连接分发方式
    bool bThreadMode                = false;        ///< 是否线程模型
    bool bAsyncLogger               = false;        ///< 是否启用异步文件日志
    bool bIsAccess                  = false;        ///< 是否接入Server
    bool bChannelVerify             = false;        ///< 是否需要连接验证
    ev_tstamp dConnectionProtection = 0.0;          ///< >0时为连接保护时间，新建连接会设置成这个时间，接收到第一个数据包之后改设成dIoTimeout
    ev_tstamp dIoTimeout            = 60.0;          ///< IO（连接）超时配置
    ev_tstamp dDataReportInterval   = 60.0;         ///< 统计数据上报时间间隔
    ev_tstamp dMsgStatInterval      = 60.0;          ///< 客户端连接发送数据包统计时间间隔
    ev_tstamp dAddrStatInterval     = 60.0;          ///< IP地址数据统计时间间隔
    ev_tstamp dStepTimeout          = 1.5;          ///< 步骤超时
    std::string strWorkPath;                        ///< 工作路径
    std::string strConfFile;                        ///< 配置文件
    std::string strNodeType;                        ///< 节点类型
    std::string strHostForServer;                   ///< 对其他Server服务的IP地址，对应 m_iS2SListenFd
    std::string strHostForClient;                   ///< 对Client服务的IP地址，对应 m_iC2SListenFd
    std::string strGateway;                         ///< 对Client服务的真实IP地址（此ip转发给m_strHostForClient）
    std::string strNodeIdentify;
};

enum E_IO_STAT
{
    IO_STAT_RECV_NUM                        = 0x0001,
    IO_STAT_RECV_BYTE                       = 0x0002,
    IO_STAT_SEND_NUM                        = 0x0004,
    IO_STAT_SEND_BYTE                       = 0x0008,
    IO_STAT_UPSTREAM_RECV_NUM               = 0x0010,
    IO_STAT_UPSTREAM_RECV_BYTE              = 0x0020,
    IO_STAT_UPSTREAM_SEND_NUM               = 0x0040,
    IO_STAT_UPSTREAM_SEND_BYTE              = 0x0080,
    IO_STAT_DOWNSTREAM_RECV_NUM             = 0x0100,
    IO_STAT_DOWNSTREAM_RECV_BYTE            = 0x0200,
    IO_STAT_DOWNSTREAM_SEND_NUM             = 0x0400,
    IO_STAT_DOWNSTREAM_SEND_BYTE            = 0x0800,
    IO_STAT_UPSTREAM_NEW_CONNECTION         = 0x1000,
    IO_STAT_UPSTREAM_DESTROY_CONNECTION     = 0x2000,
    IO_STAT_DOWNSTREAM_NEW_CONNECTION       = 0x4000,
    IO_STAT_DOWNSTREAM_DESTROY_CONNECTION   = 0x8000,
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
    uint32 uiLoad             = 0;                    ///< 负载
    uint32 uiConnection       = 0;                    ///< 连接数量
    uint32 uiRecvNum          = 0;                    ///< 接收数据包数量
    uint32 uiRecvByte         = 0;                    ///< 接收字节数
    uint32 uiSendNum          = 0;                    ///< 发送数据包数量
    uint32 uiSendByte         = 0;                    ///< 发送字节数
    uint32 uiUpStreamRecvNum                = 0;                    ///< upstream
    uint32 uiUpStreamRecvByte               = 0;                    ///< upstream
    uint32 uiUpStreamSendNum                = 0;                    ///< upstream
    uint32 uiUpStreamSendByte               = 0;                    ///< upstream
    uint32 uiDownStreamRecvNum              = 0;                    ///< downstream
    uint32 uiDownStreamRecvByte             = 0;                    ///< downstream
    uint32 uiDownStreamSendNum              = 0;                    ///< downstream
    uint32 uiDownStreamSendByte             = 0;                    ///< downstream
    uint32 uiNewUpStreamConnection          = 0;                    ///<
    uint32 uiDestroyUpStreamConnection      = 0;                    ///<
    uint32 uiNewDownStreamConnection        = 0;                    ///<
    uint32 uiDestroyDownStreamConnection    = 0;                    ///<
    ev_tstamp dBeatTime     = 0.0;                  ///< 心跳时间
    bool bStartBeatCheck    = 0.0;                  ///< 是否需要心跳检查，worker或loader进程启动时可能需要加载数据而处于繁忙状态无法响应Manager的心跳，需等待其就绪之后才开始心跳检查。

    WorkerInfo(){}
    WorkerInfo(const WorkerInfo& stAttr) = delete;
    WorkerInfo& operator=(const WorkerInfo& stAttr) = delete;
    void ResetStat()
    {
        uiLoad = 0;
        uiConnection = 0;
        uiRecvNum = 0;
        uiRecvByte = 0;
        uiSendNum = 0;
        uiSendByte = 0;
        uiUpStreamRecvNum = 0;
        uiUpStreamRecvByte = 0;
        uiUpStreamSendNum = 0;
        uiUpStreamSendByte = 0;
        uiDownStreamRecvNum = 0;
        uiDownStreamRecvByte = 0;
        uiDownStreamSendNum = 0;
        uiDownStreamSendByte = 0;
        uiNewUpStreamConnection = 0;
        uiDestroyUpStreamConnection = 0;
        uiNewDownStreamConnection = 0;
        uiDestroyDownStreamConnection = 0;
    }
};

} /* namespace neb */

#endif /* SRC_LABOR_NODEINFO_HPP_ */

