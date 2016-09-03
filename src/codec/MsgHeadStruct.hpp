/*******************************************************************************
* Project:  Nebula
* @file     MsgHeadStruct.hpp
* @brief 
* @author   Bwar
* @date:    2016年8月11日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_CODEC_MSGHEADSTRUCT_HPP_
#define SRC_CODEC_MSGHEADSTRUCT_HPP_

namespace neb
{

#pragma pack(1)

/**
 * @brief 与客户端通信消息头
 */
struct tagMsgHead
{
    unsigned char version;                  ///< 协议版本号（1字节）
    unsigned char encript;                  ///< 压缩加密算法（1字节）
    unsigned short cmd;                     ///< 命令字/功能号（2字节）
    unsigned short checksum;                ///< 校验码（2字节）
    unsigned int body_len;                  ///< 消息体长度（4字节）
    unsigned int seq;                       ///< 序列号（4字节）

    tagMsgHead() : version(0), encript(0), cmd(0), checksum(0), body_len(0), seq(0)
    {
    }
};

#pragma pack()

}

#endif /* SRC_CODEC_MSGHEADSTRUCT_HPP_ */
