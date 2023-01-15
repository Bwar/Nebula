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
#include "ios/Dispatcher.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"

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

    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), uiFromLabor, uiToLabor, uiFlags, uiStepSeq,
                std::move(const_cast<MsgHead&>(oMsgHead)), std::move(const_cast<MsgBody&>(oMsgBody))));
    }

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), pChannel, uiFlags, uiStepSeq, iCmd, uiSeq, std::move(const_cast<MsgBody&>(oMsgBody))));
    }

    E_CODEC_STATUS Encode(CBuffer* pBuff, CBuffer* pSecondlyBuff = nullptr);
    E_CODEC_STATUS Encode(int iCmd, uint32 uiSeq, const MsgBody& oMsgBody, CBuffer* pBuff);
    E_CODEC_STATUS Encode(int iCmd, uint32 uiSeq, const MsgBody& oMsgBody, CBuffer* pBuff, CBuffer* pSecondlyBuff);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody);
    E_CODEC_STATUS Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody, CBuffer* pReactBuff);

protected:
    E_CODEC_STATUS ChannelSticky(const MsgHead& oMsgHead, const MsgBody& oMsgBody);

    // request
    template<typename ...Targs>
    static int Write(uint32 uiCodecType, uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, MsgHead&& oMsgHead, MsgBody&& oMsgBody);
    // response
    template<typename ...Targs>
    static int Write(uint32 uiCodecType, std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, MsgBody&& oMsgBody);

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
    
    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), uiFromLabor, uiToLabor, uiFlags, uiStepSeq,
                std::move(const_cast<MsgHead&>(oMsgHead)), std::move(const_cast<MsgBody&>(oMsgBody))));
    }

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), pChannel, uiFlags, uiStepSeq, iCmd, uiSeq, std::move(const_cast<MsgBody&>(oMsgBody))));
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
    
    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), uiFromLabor, uiToLabor, uiFlags, uiStepSeq,
                std::move(const_cast<MsgHead&>(oMsgHead)), std::move(const_cast<MsgBody&>(oMsgBody))));
    }

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
    {
        return(CodecProto::Write(Type(), pChannel, uiFlags, uiStepSeq, iCmd, uiSeq, std::move(const_cast<MsgBody&>(oMsgBody))));
    }
};

template<typename ...Targs>
int CodecProto::Write(uint32 uiCodecType, uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, MsgHead&& oMsgHead, MsgBody&& oMsgBody)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<MsgBody, MsgHead>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(uiCodecType, uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<MsgBody, MsgHead>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, uiCodecType);
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(uiCodecType, uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

template<typename ...Targs>
int CodecProto::Write(uint32 uiCodecType, std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, int32 iCmd, uint32 uiSeq, MsgBody&& oMsgBody)
{
    uint32 uiFrom;
    uint32 uiTo;
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiCodecType, uiTo, uiFrom, uiFlags, uiStepSeq, std::forward<MsgHead>(oMsgHead), std::forward<MsgBody>(oMsgBody)));
}


} /* namespace neb */

#endif /* SRC_CODEC_CODECPROTO_HPP_ */
