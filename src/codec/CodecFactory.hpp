/*******************************************************************************
 * Project:  Nebula
 * @file     CodecFactory.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-02
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CODEC_CODECFACTORY_HPP_
#define SRC_CODEC_CODECFACTORY_HPP_

#include "codec/Codec.hpp"
#include "codec/CodecProto.hpp"
#include "codec/CodecHttp.hpp"
#include "codec/CodecPrivate.hpp"
#include "codec/CodecResp.hpp"
#include "codec/CodecRaw.hpp"
#include "codec/http2/CodecHttp2.hpp"
#include "codec/cass/CodecCass.hpp"
#include "codec/cass/CassResponse.hpp"

#include "actor/cmd/Cmd.hpp"
#include "actor/cmd/Module.hpp"
#include "actor/cmd/RedisCmd.hpp"
#include "actor/cmd/RawCmd.hpp"
#include "actor/step/PbStep.hpp"
#include "actor/step/HttpStep.hpp"
#include "actor/step/RedisStep.hpp"
#include "actor/step/RawStep.hpp"
#include "actor/step/CassStep.hpp"

namespace neb
{

class Dispatcher;
class SocketChannel;
class SelfChannel;
class SpecChannelWatcher;

class CodecFactory
{
public:
    CodecFactory();
    virtual ~CodecFactory();

    static std::shared_ptr<SocketChannel> CreateChannel(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, int iFd, E_CODEC_TYPE eCodecType, bool bIsClient, bool bWithSsl);
    static Codec* CreateCodec(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel);

    static E_CODEC_STATUS OnEvent(SpecChannelWatcher* pAsyncWatcher, std::shared_ptr<SocketChannel> pChannel);
    static E_CODEC_STATUS OnEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel);
    static bool OnSpecChannelCreated(uint32 uiCodecType, uint32 uiFromLabor, uint32 uiToLabor);

    static bool OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    static bool OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

    static bool OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg);
    static bool OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg);

    static bool OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg);
    static bool OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg);

    static bool OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize);
    static bool OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize);

    //static bool OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const CassMessage& oCassMsg);
    static bool OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const CassMessage& oCassMsg);

protected:
    static E_CODEC_STATUS OnEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, E_CODEC_STATUS eLastCodecStatus);
    static E_CODEC_STATUS OnNebulaEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart);
    static E_CODEC_STATUS OnHttpEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart);
    static E_CODEC_STATUS OnRedisEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart);
    static E_CODEC_STATUS OnCassEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart);
    static bool AutoSwitchCodec(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eOriginCodecType, uint32& uiLastCodecPos);

//    template<typename ...Targs>
//    static bool Recv(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);
//
//    template<typename ...Targs>
//    static bool Fetch(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args);

private:
    static std::vector<E_CODEC_TYPE> s_vecAutoSwitchCodec;

    friend class Dispatcher;
};

/*
template<typename ...Targs>
bool CodecFactory::Recv(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    switch (pChannel->GetCodecType())
    {
        case CODEC_PROTO:
            return(IO<CodecProto>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_NEBULA:
            return(IO<CodecNebula>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_NEBULA_IN_NODE:
            return(IO<CodecNebulaInNode>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_HTTP:
            return(IO<CodecHttp>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_HTTP2:
            return(IO<CodecHttp2>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_RESP:
            return(IO<CodecResp>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_PRIVATE:
            return(IO<CodecPrivate>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_CASS:
            return(IO<CodecCass>::Recv(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_UNKNOW:
        default:
            return(false);
    }
}

template<typename ...Targs>
bool CodecFactory::Fetch(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, Targs&&... args)
{
    switch (pChannel->GetCodecType())
    {
        case CODEC_PROTO:
            return(IO<CodecProto>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_NEBULA:
            return(IO<CodecNebula>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_NEBULA_IN_NODE:
            return(IO<CodecNebulaInNode>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_HTTP:
            return(IO<CodecHttp>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_HTTP2:
            return(IO<CodecHttp2>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_RESP:
            return(IO<CodecResp>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_PRIVATE:
            return(IO<CodecPrivate>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_CASS:
            return(IO<CodecCass>::Fetch(pDispatcher, pChannel, std::forward<Targs>(args)...));
        case CODEC_UNKNOW:
        default:
            return(false);
    }
}
*/

} /* namespace neb */

#endif /* SRC_CODEC_CODECFACTORY_HPP_ */

