/*******************************************************************************
 * Project:  Nebula
 * @file     CodecProto.hpp
 * @brief    protobuf编解码器
 * @author   Bwar
 * @date:    2016年8月11日
 * @note     对应proto里msg.proto的MsgHead和MsgBody
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECPROTO_HPP_
#define SRC_CODEC_CODECPROTO_HPP_

#include "Codec.hpp"

namespace neb
{

class CodecProto: public Codec
{
public:
    CodecProto(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel);
    virtual ~CodecProto();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_PROTO);
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff);
    E_CODEC_STATUS Encode(int iCmd, uint32 uiSeq, const MsgBody& oMsgBody, CBuffer* pBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody, CBuffer* pReactBuff);

protected:
    E_CODEC_STATUS ChannelSticky(const MsgHead& oMsgHead, const MsgBody& oMsgBody);

private:
    uint32 m_uiForeignSeq = 0;
};

class CodecNebula: public CodecProto
{
public:
    CodecNebula(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel)
        : CodecProto(pLogger, eCodecType, pBindChannel)
    {
    }
    virtual ~CodecNebula()
    {
    }

    static E_CODEC_TYPE Type()
    {
        return(CODEC_NEBULA);
    }
};

class CodecNebulaInNode: public CodecProto
{
public:
    CodecNebulaInNode(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType,
            std::shared_ptr<SocketChannel> pBindChannel)
        : CodecProto(pLogger, eCodecType, pBindChannel)
    {
    }
    virtual ~CodecNebulaInNode()
    {
    }

    static E_CODEC_TYPE Type()
    {
        return(CODEC_NEBULA_IN_NODE);
    }
};

} /* namespace neb */

#endif /* SRC_CODEC_CODECPROTO_HPP_ */
