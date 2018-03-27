/*******************************************************************************
 * Project:  Nebula
 * @file     OssCodec.hpp
 * @brief    编解码器
 * @author   Bwar
 * @date:    2016年8月10日
 * @note     编解码器基类
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODEC_HPP_
#define SRC_CODEC_CODEC_HPP_

//#include <zlib.h>
//#include <zconf.h>

#include <actor/cmd/CW.hpp>
#include "log4cplus/loggingmacros.h"
#include "util/CBuffer.hpp"
#include "pb/msg.pb.h"
#include "Error.hpp"
#include "Definition.hpp"
#include "CodecDefine.hpp"

namespace neb
{

const unsigned int gc_uiGzipBit = 0x10000000;          ///< 采用zip压缩
const unsigned int gc_uiZipBit  = 0x20000000;          ///< 采用gzip压缩
const unsigned int gc_uiRc5Bit  = 0x01000000;          ///< 采用12轮Rc5加密
const unsigned int gc_uiAesBit  = 0x02000000;          ///< 采用128位aes加密

enum E_CODEC_TYPE
{
    CODEC_UNKNOW            = 0,        ///< 未知
    CODEC_TLV               = 1,        ///< TLV编解码
    CODEC_PROTOBUF          = 2,        ///< Protobuf编解码
    CODEC_HTTP              = 3,        ///< HTTP编解码
    CODEC_HTTP_CUSTOM       = 4,        ///< 带自定义HTTP头的http编解码
    CODEC_PRIVATE           = 5,        ///< 私有协议编解码
    CODEC_WS_EXTEND_JSON    = 6,        ///< 带Extension data的websocket协议扩展，Application data为json
    CODEC_WS_EXTEND_PB      = 7,        ///< 带Extension data的websocket协议扩展，Application data为pb
};

/**
 * @brief 编解码状态
 * @note 编解码状态用于判断编解码是否成功，其中解码发生CODEC_STATUS_ERR情况时
 * 调用方需关闭对应连接；解码发生CODEC_STATUS_PAUSE时，解码函数应将缓冲区读位
 * 置重置回读开始前的位置。
 */
enum E_CODEC_STATUS
{
    CODEC_STATUS_OK         = 0,    ///< 编解码成功
    CODEC_STATUS_PAUSE      = 1,    ///< 编解码暂停（数据不完整，等待数据完整之后再解码）
    CODEC_STATUS_INVALID    = 2,    ///< 使用了错误的编解码方式（此时为调用错误，应修改调用方式）
    CODEC_STATUS_ERR        = 3,    ///< 编解码失败
    CODEC_STATUS_EOF        = 4,    ///< 连接正常关闭
    CODEC_STATUS_INT        = 5,    ///< 连接非正常关闭
};

class Codec
{
public:
    Codec(E_CODEC_TYPE eCodecType, const std::string& strKey = "That's a lizard.");
    virtual ~Codec();

    E_CODEC_TYPE GetCodecType() const
    {
        return(m_eCodecType);
    }

    /**
     * @brief 字节流编码
     * @param[in] oMsgHead  消息包头
     * @param[in] oMsgBody  消息包体
     * @param[out] pBuff  数据缓冲区
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff) = 0;

    /**
     * @brief 字节流解码
     * @param[in,out] pBuff 数据缓冲区
     * @param[out] oMsgHead 消息包头
     * @param[out] oMsgBody 消息包体
     * @return 编解码状态
     */
    virtual E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody) = 0;


protected:
    /**
     * @brief 获取日志类实例
     * @note 派生类写日志时调用
     * @return 日志类实例
     */
    log4cplus::Logger GetLogger()
    {
        return (*m_pLogger);
    }

    log4cplus::Logger* GetLoggerPtr()
    {
        return (m_pLogger);
    }

    const std::string& GetKey() const
    {
        return(m_strKey);
    }

    bool Zip(const std::string& strSrc, std::string& strDest);
    bool Unzip(const std::string& strSrc, std::string& strDest);
    bool Gzip(const std::string& strSrc, std::string& strDest);
    bool Gunzip(const std::string& strSrc, std::string& strDest);
    bool Rc5Encrypt(const std::string& strSrc, std::string& strDest);
    bool Rc5Decrypt(const std::string& strSrc, std::string& strDest);
    bool AesEncrypt(const std::string& strSrc, std::string& strDest);
    bool AesDecrypt(const std::string& strSrc, std::string& strDest);

private:
    /**
     * @brief 设置日志类实例
     * @note 设置框架层操作者，由框架层调用，业务层派生类可直接忽略此函数
     * @param logger 日志类实例
     */
    void SetLogger(log4cplus::Logger* pLogger)
    {
        m_pLogger = pLogger;
    }

private:
    log4cplus::Logger* m_pLogger;
    E_CODEC_TYPE m_eCodecType;
    std::string m_strKey;       // 密钥

    friend class SocketChannel;
};

} /* namespace neb */

#endif /* SRC_CODEC_CODEC_HPP_ */
