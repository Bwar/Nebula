/*******************************************************************************
* Project:  Nebula
* @file     Attribute.hpp
* @brief 
* @author   Bwar
* @date:    2016年8月10日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_LABOR_CHIEF_ATTRIBUTE_HPP_
#define SRC_LABOR_CHIEF_ATTRIBUTE_HPP_

#include "Definition.hpp"

namespace neb
{

class Labor;
class Manager;
class Worker;
class Cmd;
class Module;

typedef Cmd* CreateCmd();

struct tagSo
{
    void* pSoHandle;
    Cmd* pCmd;
    int iVersion;
    tagSo();
    ~tagSo();
};

struct tagModule
{
    void* pSoHandle;
    Module* pModule;
    int iVersion;
    tagModule();
    ~tagModule();
};

struct tagClientConnWatcherData
{
    in_addr_t iAddr;
    Labor* pLabor;     // 不在结构体析构时回收

    tagClientConnWatcherData() : iAddr(0), pLabor(NULL)
    {
    }
};

/**
 * @brief 工作进程属性
 */
struct tagWorkerAttr
{
    int iWorkerIndex;                   ///< 工作进程序号
    int iControlFd;                     ///< 与Manager进程通信的文件描述符（控制流）
    int iDataFd;                        ///< 与Manager进程通信的文件描述符（数据流）
    int32 iLoad;                        ///< 负载
    int32 iConnect;                     ///< 连接数量
    int32 iRecvNum;                     ///< 接收数据包数量
    int32 iRecvByte;                    ///< 接收字节数
    int32 iSendNum;                     ///< 发送数据包数量
    int32 iSendByte;                    ///< 发送字节数
    int32 iClientNum;                   ///< 客户端数量
    ev_tstamp dBeatTime;                ///< 心跳时间

    tagWorkerAttr()
    {
        iWorkerIndex = 0;
        iControlFd = -1;
        iDataFd = -1;
        iLoad = 0;
        iConnect = 0;
        iRecvNum = 0;
        iRecvByte = 0;
        iSendNum = 0;
        iSendByte = 0;
        iClientNum = 0;
        dBeatTime = 0.0;
    }

    tagWorkerAttr(const tagWorkerAttr& stAttr)
    {
        iWorkerIndex = stAttr.iWorkerIndex;
        iControlFd = stAttr.iControlFd;
        iDataFd = stAttr.iDataFd;
        iLoad = stAttr.iLoad;
        iConnect = stAttr.iConnect;
        iRecvNum = stAttr.iRecvNum;
        iRecvByte = stAttr.iRecvByte;
        iSendNum = stAttr.iSendNum;
        iSendByte = stAttr.iSendByte;
        iClientNum = stAttr.iClientNum;
        dBeatTime = stAttr.dBeatTime;
    }

    tagWorkerAttr& operator=(const tagWorkerAttr& stAttr)
    {
        iWorkerIndex = stAttr.iWorkerIndex;
        iControlFd = stAttr.iControlFd;
        iDataFd = stAttr.iDataFd;
        iLoad = stAttr.iLoad;
        iConnect = stAttr.iConnect;
        iRecvNum = stAttr.iRecvNum;
        iRecvByte = stAttr.iRecvByte;
        iSendNum = stAttr.iSendNum;
        iSendByte = stAttr.iSendByte;
        iClientNum = stAttr.iClientNum;
        dBeatTime = stAttr.dBeatTime;
        return(*this);
    }
};

}

#endif /* SRC_LABOR_CHIEF_ATTRIBUTE_HPP_ */
