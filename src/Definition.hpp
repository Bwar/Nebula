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


#ifndef NODE_BEAT
#define NODE_BEAT 1.0
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
        if(NULL != ptr) \
        {               \
            delete ptr; \
            ptr = NULL; \
        }               \
    } while(0)

#define DELETE_ARRAY(ptr) \
    do\
    {                   \
        if(NULL != ptr) \
        {               \
            delete[] ptr; \
            ptr = NULL; \
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

#define LOG4_FATAL(args...) LOG4CPLUS_FATAL_FMT(GetLogger(), ##args)
#define LOG4_ERROR(args...) LOG4CPLUS_ERROR_FMT(GetLogger(), ##args)
#define LOG4_WARN(args...) LOG4CPLUS_WARN_FMT(GetLogger(), ##args)
#define LOG4_INFO(args...) LOG4CPLUS_INFO_FMT(GetLogger(), ##args)
#define LOG4_DEBUG(args...) LOG4CPLUS_DEBUG_FMT(GetLogger(), ##args)
#define LOG4_TRACE(args...) LOG4CPLUS_TRACE_FMT(GetLogger(), ##args)

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
typedef double ev_tstamp;           // ev.h


namespace neb
{

/** @brief 心跳间隔时间（单位:秒） */
const int gc_iBeatInterval = NODE_BEAT;

/** @brief Server刷新计数， gc_iRefreshInterval * gc_iBeatInterval 为Server刷新时间 */
const int gc_iRefreshInterval = 10;

/** @brief 每次epoll_wait能处理的最大事件数  */
const int gc_iMaxEpollEvents = 100;

/** @brief 最大缓冲区大小 */
const int gc_iMaxBuffLen = 65535;

/** @brief 错误信息缓冲区大小 */
const int gc_iErrBuffLen = 256;

const uint32 gc_uiMsgHeadSize = 15;
const uint32 gc_uiClientMsgHeadSize = 14;

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
    CHANNEL_STATUS_DESTROY              = 7,    ///< 连接已被销毁
};

/**
 * @brief 消息通信上下文
 * @note 当外部请求到达时，因Server所有操作均为异步操作，无法立刻对请求作出响应，在完成若干个
 * 异步调用之后再响应，响应时需要有请求通道信息tagChannelContext。接收请求时在原消息前面加上
 * tagChannelContext，响应消息从通过tagChannelContext里的信息原路返回给请求方。若通过tagChannelContext
 * 里的信息无法找到请求路径，则表明请求方已发生故障或已超时被清理，消息直接丢弃。
 */
struct tagChannelContext
{
    int32 iFd;          ///< 请求消息来源文件描述符
    uint32 ulSeq;       ///< 文件描述符创建时对应的序列号

    tagChannelContext() : iFd(0), ulSeq(0)
    {
    }

    tagChannelContext(const tagChannelContext& stCtx) : iFd(stCtx.iFd), ulSeq(stCtx.ulSeq)
    {
    }

    tagChannelContext& operator=(const tagChannelContext& stCtx)
    {
        iFd = stCtx.iFd;
        ulSeq = stCtx.ulSeq;
        return(*this);
    }
};

}

#endif /* SRC_DEFINITION_HPP_ */
