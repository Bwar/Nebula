/*******************************************************************************
* Project:  Nebula
* @file     Error.hpp
* @brief 
* @author   Bwar
* @date:    2016年8月13日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_ERROR_HPP_
#define SRC_ERROR_HPP_

namespace neb
{

/**
 * @brief 系统错误码定义
 */
enum E_ERROR_NO
{
    ERR_OK                              = 0,        ///< 正确
    ERR_CHANNEL_EOF                     = 9001,     ///< 连接关闭
    ERR_TRANSFER_FD                     = 9002,     ///< 传输文件描述符出错(errno为0时返回)
    ERR_PARASE_PROTOBUF                 = 10000,    ///< 解析Protobuf出错
    ERR_NO_SUCH_WORKER_INDEX            = 10001,    ///< 未知的Worker进程编号
    ERR_UNKNOWN_CMD                     = 10002,    ///< 未知命令字
    ERR_NEW                             = 10003,    ///< new出错（无法分配堆内存）
    ERR_REDIS_CONN_CLOSE                = 10004,    ///< redis连接已关闭
    ERR_DISCONNECT                      = 10005,    ///< 已存在的连接发生错误
    ERR_NO_CALLBACK                     = 10006,    ///< 回调不存在或已超时
    ERR_DATA_TRANSFER                   = 10007,    ///< 数据传输出错
    ERR_REPEAT_REGISTERED               = 10008,    ///< 重复注册
    ERR_SERVERINFO                      = 10009,    ///< 服务器信息错误
    ERR_SESSION                         = 10010,    ///< 获取会话错误
    ERR_BODY_JSON                       = 10011,    ///< 消息体json解析错误
    ERR_SERVERINFO_RECORD               = 10012,    ///< 存档服务器信息错误
    ERR_NODE_NUM                        = 10013,    ///< 系统节点数超过最大限制65535
    ERR_SSL_INIT                        = 10014,    ///< 初始化SSL错误
    ERR_SSL_CTX                         = 10015,    ///< 创建SSL上下文错误 
    ERR_SSL_CERTIFICATE                 = 10016,    ///< 加载SSL证书错误
    ERR_SSL_NEW_CONNECTION              = 10017,    ///< 新建SSL连接错误
    ERR_SSL_HANDSHAKE                   = 10018,    ///< 建立SSL连接错误
    ERR_SSL_SHUTDOWN                    = 10019,    ///< 关闭SSL连接错误
    ERR_FILE_NOT_EXIST                  = 10020,    ///< 文件不存在
    ERR_CONNECTION                      = 10021,    ///< 连接错误

    /* 存储代理错误码段  11000~11999 */
    ERR_INCOMPLET_DATAPROXY_DATA        = 11001,    ///< DataProxy请求数据包不完整
    ERR_INVALID_REDIS_ROUTE             = 11002,    ///< 无效的redis路由信息
    ERR_REDIS_NODE_NOT_FOUND            = 11003,    ///< 未找到合适的redis节点
    ERR_REGISTERCALLBACK_REDIS          = 11004,    ///< 注册RedisStep错误
    ERR_REDIS_CMD                       = 11005,    ///< redis命令执行出错
    ERR_UNEXPECTED_REDIS_REPLY          = 11006,    ///< 不符合预期的redis结果
    ERR_RESULTSET_EXCEED                = 11007,    ///< 数据包超过protobuf最大限制
    ERR_LACK_CLUSTER_INFO               = 11008,    ///< 缺少集群信息
    ERR_TIMEOUT                         = 11009,    ///< 超时
    ERR_REDIS_AND_DB_CMD_NOT_MATCH      = 11010,    ///< redis读写操作与DB读写操作不匹配
    ERR_REDIS_NIL_AND_DB_FAILED         = 11011,    ///< redis结果集为空，但发送DB操作失败
    ERR_NO_RIGHT                        = 11012,    ///< 数据操作权限不足
    ERR_QUERY                           = 11013,    ///< 查询出错，如拼写SQL错误
    ERR_REDIS_STRUCTURE_WITH_DATASET    = 11014,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求与存储不符
    ERR_REDIS_STRUCTURE_WITHOUT_DATASET = 11015,    ///< redis数据结构并非由DB的各字段值序列化（或串联）而成，请求与存储不符
    ERR_DB_FIELD_NUM                    = 11016,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求的字段数量错误
    ERR_DB_FIELD_ORDER_OR_FIELD_NAME    = 11017,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，请求字段顺序或字段名错误
    ERR_KEY_FIELD                       = 11018,    ///< redis数据结构由DB的各字段值序列化（或串联）而成，指定的key_field错误或未指定，或未在对应表的数据字典中找到该字段
    ERR_KEY_FIELD_VALUE                 = 11019,    ///< redis数据结构指定的key_field所对应的值缺失或值为空
    ERR_JOIN_FIELDS                     = 11020,    ///< redis数据结构由DB字段串联而成，串联的字段错误
    ERR_LACK_JOIN_FIELDS                = 11021,    ///< redis数据结构由DB字段串联而成，缺失串联字段
    ERR_REDIS_STRUCTURE_NOT_DEFINE      = 11022,    ///< redis数据结构未在DataProxy的配置中定义
    ERR_INVALID_CMD_FOR_HASH_DATASET    = 11023,    ///< redis hash数据结构由DB的各字段值序列化（或串联）而成，而请求中的hash命令不当
    ERR_DB_TABLE_NOT_DEFINE             = 11024,    ///< 表未在DataProxy的配置中定义
    ERR_DB_OPERATE_MISSING              = 11025,    ///< redis数据结构存在对应的DB表，但数据请求缺失对数据库表操作
    ERR_REQ_MISS_PARAM                  = 11026,    ///< 请求参数缺失
    ERR_LACK_TABLE_FIELD                = 11027,    ///< 数据库表字段缺失
    ERR_TABLE_FIELD_NAME_EMPTY          = 11028,    ///< 数据库表字段名为空
    ERR_UNDEFINE_REDIS_OPERATE          = 11029,    ///< 未定义或不支持的redis数据操作（只支持string和hash的dataset update操作）
    ERR_REDIS_READ_WRITE_CMD_NOT_MATCH  = 11030,    ///< redis读命令与写命令不对称
};

}

#endif /* SRC_ERROR_HPP_ */
