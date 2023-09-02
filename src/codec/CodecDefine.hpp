/*******************************************************************************
* Project:  Nebula
* @file     MsgHeadStruct.hpp
* @brief 
* @author   Bwar
* @date:    2016年8月11日
* @note
* Modify history:
******************************************************************************/
#ifndef SRC_CODEC_CODECDEFINE_HPP_
#define SRC_CODEC_CODECDEFINE_HPP_

namespace neb
{

const uint8 WEBSOCKET_FIN                   = 0x80;
const uint8 WEBSOCKET_RSV1                  = 0x40;
const uint8 WEBSOCKET_RSV2                  = 0x20;
const uint8 WEBSOCKET_RSV3                  = 0x10;
const uint8 WEBSOCKET_OPCODE                = 0x0F;
const uint8 WEBSOCKET_FRAME_CONTINUE        = 0;
const uint8 WEBSOCKET_FRAME_TEXT            = 1;
const uint8 WEBSOCKET_FRAME_BINARY          = 2;
// %x3-7 保留用于未来的非控制帧
const uint8 WEBSOCKET_FRAME_CLOSE           = 8;
const uint8 WEBSOCKET_FRAME_PING            = 9;
const uint8 WEBSOCKET_FRAME_PONG            = 10;
// %xB-F 保留用于未来的控制帧
const uint8 WEBSOCKET_MASK                  = 0x80;
const uint8 WEBSOCKET_PAYLOAD_LEN           = 0x7F;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT16    = 126;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT64    = 127;

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

#endif /* SRC_CODEC_CODECDEFINE_HPP_ */
