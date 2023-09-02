/*******************************************************************************
 * Project:  neb
 * @file     StreamCoder.hpp
 * @brief 
 * @author   bwarliao
 * @date:    2014年9月10日
 * @note
 * Modify history:
 ******************************************************************************/
#ifndef STREAMCODER_HPP_
#define STREAMCODER_HPP_

namespace neb
{

enum E_STREAM_CODEC_TYPE
{
    STREAM_CODEC_UNKNOW            = 0,        ///< 未知
    STREAM_CODEC_TLV               = 1,        ///< TLV编解码
    STREAM_CODEC_PROTOBUF          = 2,        ///< Protobuf编解码
    STREAM_CODEC_HTTP              = 3,        ///< HTTP编解码
    STREAM_CODEC_HTTP_CUSTOM       = 4,        ///< 带自定义HTTP头的http编解码
};

class CStreamCodec
{
public:
    CStreamCodec(E_STREAM_CODEC_TYPE eCodecType)
        : m_eCodecType(eCodecType)
    {
    };
    virtual ~CStreamCodec(){};

    E_STREAM_CODEC_TYPE GetCodecType() const
    {
        return(m_eCodecType);
    }

private:
    E_STREAM_CODEC_TYPE m_eCodecType;
};

}

#endif /* STREAMCODER_HPP_ */
