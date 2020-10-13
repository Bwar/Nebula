/*******************************************************************************
 * Project:  Nebula
 * @file     H2Comm.hpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-02
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_HTTP2_H2COMM_HPP_
#define SRC_CODEC_HTTP2_H2COMM_HPP_

namespace neb
{

const uint32 H2_FRAME_HEAD_SIZE = 72;
const uint32 SETTINGS_MAX_FRAME_SIZE = (2^24) - 1;
const uint32 DEFAULT_SETTINGS_MAX_FRAME_SIZE = (2^14);
const uint32 SETTINGS_MAX_INITIAL_WINDOW_SIZE = (2^31) - 1;
const uint32 DEFAULT_SETTINGS_MAX_INITIAL_WINDOW_SIZE = (2^16) - 1;

/*
 * @see https://httpwg.org/specs/rfc7540.html#FRAME_SIZE_ERROR
 */
enum E_H2_ERR_CODE
{
    H2_ERR_NO_ERROR                 = 0x0,
    H2_ERR_PROTOCOL_ERROR           = 0x1,
    H2_ERR_INTERNAL_ERROR           = 0x2,
    H2_ERR_FLOW_CONTROL_ERROR       = 0x3,
    H2_ERR_SETTINGS_TIMEOUT         = 0x4,
    H2_ERR_STREAM_CLOSED            = 0x5,
    H2_ERR_FRAME_SIZE_ERROR         = 0x6,
    H2_ERR_REFUSED_STREAM           = 0x7,
    H2_ERR_CANCEL                   = 0x8,
    H2_ERR_COMPRESSION_ERROR        = 0x9,
    H2_ERR_CONNECT_ERROR            = 0xa,
    H2_ERR_ENHANCE_YOUR_CALM        = 0xb,
    H2_ERR_INADEQUATE_SECURITY      = 0xc,
    H2_ERR_HTTP_1_1_REQUIRED        = 0xd,
};

enum E_H2_DATA_MASK
{
    H2_DATA_MASK_1_BYTE_HIGHEST_BIT = 0x80,
    H2_DATA_MASK_1_BYTE_LOW_7_BIT   = 0x7F,
    H2_DATA_MASK_4_BYTE_HIGHEST_BIT = 0x80000000,
    H2_DATA_MASK_4_BYTE_LOW_31_BIT  = 0x7FFFFFFF,
};

enum E_H2_SETTING_REGISTRY
{
    H2_SETTINGS_HEADER_TABLE_SIZE           = 0x1,
    H2_SETTINGS_ENABLE_PUSH                 = 0x2,
    H2_SETTINGS_MAX_CONCURRENT_STREAMS      = 0x3,
    H2_SETTINGS_INITIAL_WINDOW_SIZE         = 0x4,
    H2_SETTINGS_MAX_FRAME_SIZE              = 0x5,
    H2_SETTINGS_MAX_HEADER_LIST_SIZE        = 0x6,
};

struct tagH2FrameHead
{
    char cR                         = 0;
    uint8 ucType                    = 0; ///< 帧的8位类型。帧类型决定帧的格式和语义。实现必须忽略并丢弃任何类型未知的帧。
    uint8 ucFlag                    = 0; ///< 为帧类型专用的布尔标志保留的8位字段。标志被分配特定于指定帧类型的语义。没有为特定帧类型定义语义的标志务必被忽略，并且在发送时务必保持未设置（0x0）。                        ///< 保留的1位字段。该位的语义是未定义的，并且该位必须在发送时保持未设置（0x0），并且在接收时必须忽略。
    uint32 uiLength                 = 0; ///< 帧有效载荷的长度，表示为无符号的24位整数。除非接收方为SETTINGS_MAX_FRAME_SIZE设置了较大的值，否则不得发送大于2 ^ 14（16,384）的值。帧头的9个八位字节不包含在该值中。
    uint32 uiStreamIdentifier       = 0; ///< 流标识符，表示为一个无符号的31位整数。值0x0保留给与整个连接相关联的帧，而不是单个流。
};

struct tagPriority
{
    char E                  = 0;
    uint8 ucWeight          = 0;
    uint32 uiDependency     = 0;

    tagPriority(const tagPriority& stFrom)
    {
        E = stFrom.E;
        ucWeight = stFrom.ucWeight;
        uiDependency = stFrom.uiDependency;
    }

    tagPriority& operator=(const tagPriority& stFrom)
    {
        E = stFrom.E;
        ucWeight = stFrom.ucWeight;
        uiDependency = stFrom.uiDependency;
        return(*this);
    }
};

struct tagSetting
{
    uint16 unIdentifier     = 0;
    uint32 uiValue          = 0;

    tagSetting(const tagSetting& stSetting)
    {
        unIdentifier = stSetting.unIdentifier;
        uiValue = stSetting.uiValue;
    }

    tagSetting& operator=(const tagSetting& stSetting)
    {
        unIdentifier = stSetting.unIdentifier;
        uiValue = stSetting.uiValue;
        return(*this);
    }
};

}

#endif /* SRC_CODEC_HTTP2_H2COMM_HPP_ */

