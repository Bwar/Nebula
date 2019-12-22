/*******************************************************************************
* Project:  Nebula
* @file     CW.hpp
* @brief    保留命令字定义
* @author   Bwar
* @date:    2016年8月10日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_ACTOR_CMD_CW_HPP_
#define SRC_ACTOR_CMD_CW_HPP_

namespace neb
{

const unsigned int gc_uiCmdReq = 0x00000001;          ///< 请求（奇数为请求，偶数为应答，通过 gc_iCmdReq & iCmd 来判断，比%运算快）
const unsigned int gc_uiCmdBit = 0x0000FFFF;          ///< 命令为无符号短整型

/**
 * @brief 保留命令字
 * @note 小于等于500的命令字为保留命令字，业务层禁止使用500及以下的命令字，建议业务层命令字从1001开始编号
 * 并且遵从奇数表示请求命令字，偶数表示应答命令字，应答命令字 = 请求命令字 + 1
 */
enum E_CMD
{
    CMD_UNDEFINE                        = 0,    ///< 未定义
    CMD_REQ_CONNECT_TO_WORKER           = 1,    ///< 连接到Worker进程请求（Manager进程在收到这个包之后才会将文件描述符转发给对应Worker进程）
    CMD_RSP_CONNECT_TO_WORKER           = 2,    ///< 连接到Worker进程应答（无须响应）
    CMD_REQ_TELL_WORKER                 = 3,    ///< 将己方Worker信息告知对方，并请求对方作对等的回应
    CMD_RSP_TELL_WORKER                 = 4,    ///< 收到对方Worker告知的信息，并将己方的对等信息回应之
    CMD_REQ_DISCONNECT                  = 5,    ///< 连接断开（由框架层触发通知并以Cmd的形式通知到系统Cmd，断开原因有error、timeout之类）
    CMD_RSP_DISCONNECT                  = 6,    ///< 连接断开应答（无效CMD）
    CMD_REQ_REFRESH_SERVER              = 7,    ///< 刷新Server配置请求（由Manager进程根据配置文件变化情况刷新Server配置，如修改日志级别，重新加载动态库等）
    CMD_RSP_REFRESH_SERVER              = 8,    ///< 刷新Server配置应答
    CMD_REQ_SET_LOG_LEVEL               = 9,    ///< 设置日志级别请求（manager to worker）
    CMD_RSP_SET_LOG_LEVEL               = 10,   ///< 设置日志级别响应（无须响应）
    CMD_REQ_RELOAD_SO                   = 11,   ///< 重新加载so请求（manager to worker）
    CMD_RSP_RELOAD_SO                   = 12,   ///< 重新加载so响应（无须响应）
    CMD_REQ_UPDATE_WORKER_LOAD          = 13,   ///< 更新Worker进程负载信息请求
    CMD_RSP_UPDATE_WORKER_LOAD          = 14,   ///< 更新Worker进程负载信息应答（一般无须应答）

    CMD_REQ_NODE_STATUS_REPORT          = 101,  ///< 节点Server状态上报请求（各节点向控制中心上报自身状态信息）
    CMD_RSP_NODE_STATUS_REPORT          = 102,  ///< 节点Server状态上报应答
    CMD_REQ_NODE_REGISTER               = 103,  ///< 节点注册,各个节点启动时主动注册到Center模块
    CMD_RSP_NODE_REGISTER               = 104,  ///< 节点注册应答
    CMD_REQ_NODE_NOTICE                 = 105,  ///< 节点通知,各个节点启动时主动注册到Center模块，发的通知
    CMD_RSP_NODE_NOTICE                 = 106,  ///< 节点通知应答通知（Manager应答，Worker无须应答）
    CMD_REQ_REFRESH_NODE_ID             = 107,  ///< 更新节点ID请求（Manager发往Worker）
    CMD_RSP_REFRESH_NODE_ID             = 108,  ///< 更新节点ID应答（一般无须应答）
    CMD_REQ_DATA_REPORT                 = 109,  ///< 服务器数据状态上报请求
    CMD_RSP_DATA_REPORT                 = 110,  ///< 服务器数据状态上报应答
    CMD_REQ_GET_LOAD_MIN_SERVER         = 111,  ///< 获取低负载服务器请求
    CMD_RSP_GET_LOAD_MIN_SERVER         = 112,  ///< 获取低负载服务器应答
    CMD_REQ_LEADER_ELECTION             = 113,  ///< 分布式leader选举请求
    CMD_RSP_LEADER_ELECTION             = 114,  ///< 分布式leader选举响应（通过心跳结合优先权选举，无须响应）

    CMD_REQ_SET_NODE_CONFIG             = 201,  ///< 更新框架配置文件请求
    CMD_RSP_SET_NODE_CONFIG             = 202,  ///< 更新框架配置文件应答
    CMD_REQ_GET_NODE_CONFIG             = 203,  ///< 获取框架配置文件请求
    CMD_RSP_GET_NODE_CONFIG             = 204,  ///< 获取框架配置文件应答
    CMD_REQ_SET_NODE_CUSTOM_CONFIG      = 205,  ///< 更新框架配置文件中自定义部分配置请求
    CMD_RSP_SET_NODE_CUSTOM_CONFIG      = 206,  ///< 更新框架配置文件中自定义部分配置应答
    CMD_REQ_GET_NODE_CUSTOM_CONFIG      = 207,  ///< 获取框架配置文件中自定义部分配置请求
    CMD_RSP_GET_NODE_CUSTOM_CONFIG      = 208,  ///< 获取框架配置文件中自定义部分配置应答
    CMD_REQ_SET_CUSTOM_CONFIG           = 209,  ///< 更新自定义配置文件请求
    CMD_RSP_SET_CUSTOM_CONFIG           = 210,  ///< 更新自定义配置文件响应
    CMD_REQ_GET_CUSTOM_CONFIG           = 211,  ///< 获取自定义配置文件请求
    CMD_RSP_GET_CUSTOM_CONFIG           = 212,  ///< 获取自定义配置文件响应
    CMD_REQ_RELOAD_CUSTOM_CONFIG        = 213,  ///< 重新加载自定义配置文件请求
    CMD_RSP_RELOAD_CUSTOM_CONFIG        = 214,  ///< 重新加载自定义配置文件响应

    CMD_REQ_LOG4_TRACE                  = 401,  ///< 分布式网络日志请求
    CMD_RSP_LOG4_TRACE                  = 402,  ///< 分布式网络日志响应（无须响应）

    // 接入层转发命令字，如客户端数据转发给Logic，Logic数据转发给客户端等
    CMD_REQ_FROM_CLIENT                 = 501,  ///< 客户端发送过来需由接入层转发的数据，传输的MsgHead里的Cmd不会被改变（无业务逻辑直接转发的场景，如登录等接入层有业务逻辑的场景不适用）
    CMD_RSP_FROM_CLIENT                 = 502,  ///< 无意义，不会被使用
    CMD_REQ_TO_CLIENT                   = 503,  ///< 往客户端发送的数据，传输的MsgHead里的Cmd不会被改变（直接转发，不需要做任何处理）
    CMD_RSP_TO_CLIENT                   = 504,  ///< 无意义，不会被使用
    CMD_REQ_STORATE                     = 505,  ///< 存储请求
    CMD_RSP_STORATE                     = 506,  ///< 存储响应
    CMD_REQ_BEAT                        = 507,  ///< 心跳请求
    CMD_RSP_BEAT                        = 508,  ///< 心跳响应
    CMD_REQ_LOCATE_STORAGE              = 511,  ///< 定位数据存储位置请求
    CMD_RSP_LOCATE_STORAGE              = 512,  ///< 定位数据存储位置响应
    CMD_REQ_SYS_ERROR                   = 999,  ///< 系统错误请求（无意义，不会被使用）
    CMD_RSP_SYS_ERROR                   = 1000, ///< 系统错误响应

};

}

#endif /* SRC_ACTOR_CMD_CW_HPP_ */
