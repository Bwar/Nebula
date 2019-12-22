/*******************************************************************************
* Project:  Nebula
* @file     Definition.hpp
* @brief 
* @author   Bwar
* @date:    2016年8月10日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_DEFINITION_HPP_
#define SRC_DEFINITION_HPP_

#include <stddef.h>

#ifndef NODE_BEAT
#define NODE_BEAT 7.0
#endif

/*
#define NEW(type) \
    do\
    { \
        try                  \
        {                    \
           return(new type);   \
        }                    \
        catch(std::bad_alloc)\
        {                    \
            return NULL;      \
        }                    \
    } while(0)

#define NEW_PTR(ptr,type) do\
    { \
        try                  \
        {                    \
           ptr = new type;   \
        }                    \
        catch(std::bad_alloc)\
        {                    \
            ptr = NULL;      \
        }                    \
    }while(0)
*/

#define DELETE(ptr)     \
    do\
    {                   \
        if(nullptr != ptr) \
        {               \
            delete ptr; \
            ptr = nullptr; \
        }               \
    } while(0)

#define DELETE_ARRAY(ptr) \
    do\
    {                   \
        if(nullptr != ptr) \
        {               \
            delete[] ptr; \
            ptr = nullptr; \
        }               \
    } while(0)

#define DELETE_REF(ref) \
    do\
    {                   \
        delete &ref ;   \
    } while(0)

/*
#define MALLOC(size)\
    do\
    {                   \
        if(size > 0) \
        {               \
            return malloc(size); \
        }               \
        else \
        {\
            return NULL; \
        }\
    } while(0)
*/

#define FREE(ptr)\
    do\
    {                   \
        if(NULL != ptr) \
        {               \
            free(ptr); \
            ptr = NULL; \
        }               \
    } while(0)

#define LOG4_FATAL(args...) Logger(neb::Logger::FATAL, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_ERROR(args...) Logger(neb::Logger::ERROR, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_WARNING(args...) Logger(neb::Logger::WARNING, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_NOTICE(args...) Logger(neb::Logger::NOTICE, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_INFO(args...) Logger(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_CRITICAL(args...) Logger(neb::Logger::CRITICAL, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_DEBUG(args...) Logger(neb::Logger::DEBUG, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG4_TRACE(args...) Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, ##args)
//#define LOG4_TRACE(...) Logger(neb::Logger::TRACE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
typedef double ev_tstamp;           // ev.h

#ifdef __CYGWIN__
/* TCP socket options */
//#define TCP_NODELAY     1   /* Turn off Nagle's algorithm. */
#define TCP_MAXSEG      2   /* Limit MSS */
#define TCP_CORK        3   /* Never send partially complete segments */
#define TCP_KEEPIDLE        4   /* Start keeplives after this period */
#define TCP_KEEPINTVL       5   /* Interval between keepalives */
#define TCP_KEEPCNT     6   /* Number of keepalives before death */
#define TCP_SYNCNT      7   /* Number of SYN retransmits */
#define TCP_LINGER2     8   /* Life time of orphaned FIN-WAIT-2 state */
#define TCP_DEFER_ACCEPT    9   /* Wake up listener only when data arrive */
#define TCP_WINDOW_CLAMP    10  /* Bound advertised window */
#define TCP_INFO        11  /* Information about this connection. */
#define TCP_QUICKACK        12  /* Block/reenable quick acks */
#define TCP_CONGESTION      13  /* Congestion control algorithm */
#define TCP_MD5SIG      14  /* TCP MD5 Signature (RFC2385) */
#define TCP_THIN_LINEAR_TIMEOUTS 16      /* Use linear timeouts for thin streams*/
#define TCP_THIN_DUPACK         17      /* Fast retrans. after 1 dupack */
#define TCP_USER_TIMEOUT    18  /* How long for loss retry before timeout */
#define TCP_REPAIR      19  /* TCP sock is under repair right now */
#define TCP_REPAIR_QUEUE    20
#define TCP_QUEUE_SEQ       21
#define TCP_REPAIR_OPTIONS  22
#define TCP_FASTOPEN        23  /* Enable FastOpen on listeners */
#define TCP_TIMESTAMP       24
#define TCP_NOTSENT_LOWAT   25  /* limit number of unsent bytes in write queue */
#define TCP_REPAIR_WINDOW   29  /* Get/set window parameters */
#endif

namespace neb
{

/** @brief 心跳间隔时间（单位:秒） */
const int gc_iBeatInterval = NODE_BEAT;

/** @brief 每次epoll_wait能处理的最大事件数  */
const int gc_iMaxEpollEvents = 100;

/** @brief 最大缓冲区大小 */
const int gc_iMaxBuffLen = 65535;

/** @brief 错误信息缓冲区大小 */
const int gc_iErrBuffLen = 256;

/** @brief IP地址长度 */
const int gc_iAddrLen = 64;

const uint32 gc_uiMsgHeadSize = 15;
const uint32 gc_uiClientMsgHeadSize = 14;

const ev_tstamp gc_dNoTimeout = -1;
const ev_tstamp gc_dDefaultTimeout = 0;

/**
 * @brief 命令执行状态
 */
enum E_CMD_STATUS
{
    CMD_STATUS_START                    = 0,    ///< 创建命令执行者，但未开始执行
    CMD_STATUS_RUNNING                  = 1,    ///< 正在执行命令
    CMD_STATUS_COMPLETED                = 2,    ///< 命令执行完毕
    CMD_STATUS_OK                       = 3,    ///< 单步OK，但命令最终状态仍需调用者判断
    CMD_STATUS_FAULT                    = 4,    ///< 命令执行出错并且不必重试
};

/**
 * @brief 通信通道状态
 */
enum E_CHANNEL_STATUS
{
    CHANNEL_STATUS_INIT                 = 0,    ///< 初始化完毕，尚未连接 socket()返回
    CHANNEL_STATUS_TRY_CONNECT          = 1,    ///< 发起连接
    CHANNEL_STATUS_CONNECTED            = 2,    ///< 连接成功
    CHANNEL_STATUS_TRANSFER_TO_WORKER   = 3,    ///< 连接从Manager传送给Worker过程中
    CHANNEL_STATUS_WORKER               = 4,    ///< 连接成功从Manager传送给Worker
    CHANNEL_STATUS_TELL_WORKER          = 5,    ///< 将本Worker信息告知对端Worker
    CHANNEL_STATUS_ESTABLISHED          = 6,    ///< 与对端Worker的连接就绪（可以正常收发消息）
    CHANNEL_STATUS_CLOSED               = 7,    ///< 被丢弃待回收
};

}

#endif /* SRC_DEFINITION_HPP_ */
