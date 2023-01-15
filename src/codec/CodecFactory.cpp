/*******************************************************************************
 * Project:  Nebula
 * @file     CodecFactory.hpp
 * @brief    
 * @author   Bwar
 * @date:    2021-10-02
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CodecFactory.hpp"
#include "ios/IO.hpp"
#include "channel/SocketChannel.hpp"
#include "channel/SocketChannelImpl.hpp"
#include "channel/SocketChannelSslImpl.hpp"
#include "channel/SpecChannel.hpp"
#include "channel/migrate/SocketChannelPack.hpp"
#include "channel/migrate/SocketChannelMigrate.hpp"
#include "labor/LaborShared.hpp"
#include "actor/cmd/sys_cmd/CmdChannelMigrate.hpp"

namespace neb
{

std::vector<E_CODEC_TYPE> CodecFactory::s_vecAutoSwitchCodec = {
        CODEC_UNKNOW, CODEC_HTTP, CODEC_PROTO, CODEC_RESP, CODEC_HTTP2
};

CodecFactory::CodecFactory()
{
}

CodecFactory::~CodecFactory()
{
}

std::shared_ptr<SocketChannel> CodecFactory::CreateChannel(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, int iFd, E_CODEC_TYPE eCodecType, bool bIsClient, bool bWithSsl)
{
    Codec* pCodec = nullptr;
    auto pChannel = std::make_shared<SocketChannel>(pLogger, bIsClient, bWithSsl);
    ev_tstamp dKeepAlive = 10.0;
    if (pLabor->GetNodeInfo().dConnectionProtection > 0)
    {
        dKeepAlive = pLabor->GetNodeInfo().dConnectionProtection;
    }
    else
    {
        dKeepAlive = pLabor->GetNodeInfo().dIoTimeout;
    }
    switch (eCodecType)
    {
        case CODEC_NEBULA:
        case CODEC_PROTO:
        case CODEC_NEBULA_IN_NODE:
            pCodec = new CodecProto(pLogger, eCodecType, pChannel);
            if (pChannel->WithSsl())
            {
#ifdef WITH_OPENSSL
                auto pImpl = std::make_shared<SocketChannelSslImpl<CodecProto>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
#else
                auto pImpl = std::make_shared<SocketChannelImpl<CodecProto>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
#endif
            }
            else
            {
                auto pImpl = std::make_shared<SocketChannelImpl<CodecProto>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
            }
            break;
        case CODEC_HTTP:
            pCodec = new CodecHttp(pLogger, eCodecType, pChannel);
            if (pChannel->WithSsl())
            {
#ifdef WITH_OPENSSL
                auto pImpl = std::make_shared<SocketChannelSslImpl<CodecHttp>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
#else
                auto pImpl = std::make_shared<SocketChannelImpl<CodecHttp>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
#endif
            }
            else
            {
                auto pImpl = std::make_shared<SocketChannelImpl<CodecHttp>>(
                        pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
                pImpl->SetCodec(pCodec);
                pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
            }
            break;
        case CODEC_HTTP2:
        {
            pCodec = new CodecHttp2(pLogger, eCodecType, pChannel);
            auto pImpl = std::make_shared<SocketChannelImpl<CodecHttp2>>(
                    pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
            pImpl->SetCodec(pCodec);
            pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
        }
            break;
        case CODEC_RESP:
        {
            pCodec = new CodecResp(pLogger, eCodecType, pChannel);
            auto pImpl = std::make_shared<SocketChannelImpl<CodecResp>>(
                    pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
            pImpl->SetCodec(pCodec);
            pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
        }
            break;
        case CODEC_PRIVATE:
        {
            pCodec = new CodecPrivate(pLogger, eCodecType, pChannel);
            auto pImpl = std::make_shared<SocketChannelImpl<CodecPrivate>>(
                    pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
            pImpl->SetCodec(pCodec);
            pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
        }
            break;
        case CODEC_CASS:
        {
            pCodec = new CodecCass(pLogger, eCodecType, pChannel);
            auto pImpl = std::make_shared<SocketChannelImpl<CodecCass>>(
                    pLabor, pLogger, bIsClient, bWithSsl, iFd, pLabor->GetSequence(), dKeepAlive);
            pImpl->SetCodec(pCodec);
            pChannel->m_pImpl = std::dynamic_pointer_cast<SocketChannel>(pImpl);
        }
            break;
        case CODEC_UNKNOW:
            return(nullptr);
        default:
            return(nullptr);
    }
    return(pChannel);
}

Codec* CodecFactory::CreateCodec(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel)
{
    Codec* pCodec = nullptr;
    switch (eCodecType)
    {
        case CODEC_NEBULA:
        case CODEC_PROTO:
        case CODEC_NEBULA_IN_NODE:
            pCodec = new CodecProto(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_HTTP:
            pCodec = new CodecHttp(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_HTTP2:
            pCodec = new CodecHttp2(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_RESP:
            pCodec = new CodecResp(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_PRIVATE:
            pCodec = new CodecPrivate(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_CASS:
            pCodec = new CodecCass(pLogger, eCodecType, pBindChannel);
            break;
        case CODEC_UNKNOW:
            break;
        default:
            //LOG4_ERROR("no codec defined for code type %d", eCodecType);
            break;
    }
    return(pCodec);
}

E_CODEC_STATUS CodecFactory::OnEvent(SpecChannelWatcher* pAsyncWatcher, std::shared_ptr<SocketChannel> pChannel)
{
    uint32 uiFlags = 0;
    uint32 uiStepSeq = 0;
    switch (pAsyncWatcher->GetCodecType())
    {
        case CODEC_PROTO:
        case CODEC_NEBULA:
        case CODEC_NEBULA_IN_NODE:
        {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->m_pLastActivityChannel = pChannel;
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oMsgHead, oMsgBody))
            {
                if (gc_uiCmdReq & oMsgHead.cmd())
                {
                    IO<Cmd>::OnMessage(pDispatcher, pChannel, oMsgHead, oMsgBody);
                }
                else
                {
                    IO<PbStep>::OnMessage(pDispatcher, pChannel, oMsgHead, oMsgBody);
                }
            }
        }
            break;
        case CODEC_HTTP:
        case CODEC_HTTP2:
        {
            HttpMsg oHttpMsg;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<HttpMsg>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->m_pLastActivityChannel = pChannel;
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oHttpMsg))
            {
                if (gc_uiCmdReq & uiFlags)
                {
                    IO<Module>::OnRequest(pDispatcher, pChannel, oHttpMsg.path(), oHttpMsg);
                }
                else
                {
                    IO<HttpStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oHttpMsg);
                }
            }
        }
            break;
        case CODEC_RESP:
        {
            RedisMsg oRedisMsg;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<RedisMsg>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->m_pLastActivityChannel = pChannel;
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oRedisMsg))
            {
                if (gc_uiCmdReq & uiFlags)
                {
                    IO<RedisCmd>::OnRequest(pDispatcher, pChannel, (int32)CMD_REQ_REDIS_PROXY, oRedisMsg);
                }
                else
                {
                    IO<RedisStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oRedisMsg);
                }
            }
        }
            break;
        case CODEC_CASS:
        {
            CassResponse oCassResponse;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<CassResponse>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->m_pLastActivityChannel = pChannel;
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oCassResponse))
            {
                IO<CassStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oCassResponse);
            }
        }
            break;
        case CODEC_PRIVATE:
            break;
        case CODEC_CHANNEL_MIGRATE:
        {
            SocketChannelPack oPack;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<SocketChannelPack>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->m_pLastActivityChannel = pChannel;
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oPack))
            {
                if (gc_uiCmdReq & uiFlags)
                {
                    IO<CmdChannelMigrate>::OnRequest(pDispatcher, pChannel, (int32)CMD_REQ_CHANNEL_MIGRATE, oPack);
                }
                else
                {
                    ;   // no response
                }
            }
        }
            break;
        case CODEC_UNKNOW:
            break;
        default:
            ;
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS CodecFactory::OnEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel)
{
    uint32 uiLastCodecPos = 0;
    E_CODEC_TYPE eOriginCodecType = pChannel->GetCodecType();
    E_CODEC_STATUS eCodecStatus = OnEvent(pDispatcher, pChannel, CODEC_STATUS_OK);
    while (CODEC_STATUS_INVALID == eCodecStatus)
    {
        if (!AutoSwitchCodec(pDispatcher, pChannel, eOriginCodecType, uiLastCodecPos))
        {
            eCodecStatus = CODEC_STATUS_ERR;
            break;
        }
        eCodecStatus = OnEvent(pDispatcher, pChannel, CODEC_STATUS_INVALID);
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecFactory::OnEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, E_CODEC_STATUS eLastCodecStatus)
{
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_ERR;
    int iStart = 0;
    if (CODEC_STATUS_INVALID == eLastCodecStatus)
    {
        iStart = 1;
    }
    switch (pChannel->GetCodecType())
    {
        case CODEC_PROTO:
        case CODEC_NEBULA:
        case CODEC_NEBULA_IN_NODE:
            eCodecStatus = OnNebulaEvent(pDispatcher, pChannel, iStart);
            break;
        case CODEC_HTTP:
        case CODEC_HTTP2:
            eCodecStatus = OnHttpEvent(pDispatcher, pChannel, iStart);
            break;
        case CODEC_RESP:
            eCodecStatus = OnRedisEvent(pDispatcher, pChannel, iStart);
            break;
        case CODEC_CASS:
            eCodecStatus = OnCassEvent(pDispatcher, pChannel, iStart);
            break;
        case CODEC_PRIVATE:
            break;
        case CODEC_UNKNOW:
            break;
        default:
            ;
    }
    return(eCodecStatus);
}

bool CodecFactory::OnSpecChannelCreated(uint32 uiCodecType, uint32 uiFromLabor, uint32 uiToLabor)
{
    auto pChannel = LaborShared::Instance()->GetSpecChannel(uiCodecType, uiFromLabor, uiToLabor);
    uint32 uiFlags = 0;
    uint32 uiStepSeq = 0;
    switch (uiCodecType)
    {
        case CODEC_PROTO:
        case CODEC_NEBULA:
        case CODEC_NEBULA_IN_NODE:
        {
            MsgHead oMsgHead;
            MsgBody oMsgBody;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->AddEvent(pSpecChannel->MutableWatcher()->MutableAsyncWatcher(), Dispatcher::AsyncCallback);
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oMsgHead, oMsgBody))
            {
                if (gc_uiCmdReq & oMsgHead.cmd())
                {
                    IO<Cmd>::OnMessage(pDispatcher, pChannel, oMsgHead, oMsgBody);
                }
                else
                {
                    IO<PbStep>::OnMessage(pDispatcher, pChannel, oMsgHead, oMsgBody);
                }
            }
        }
            break;
        case CODEC_HTTP:
        case CODEC_HTTP2:
        {
            HttpMsg oHttpMsg;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<HttpMsg>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->AddEvent(pSpecChannel->MutableWatcher()->MutableAsyncWatcher(), Dispatcher::AsyncCallback);
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oHttpMsg))
            {
                if (gc_uiCmdReq & uiFlags)
                {
                    IO<Module>::OnRequest(pDispatcher, pChannel, oHttpMsg.path(), oHttpMsg);
                }
                else
                {
                    IO<HttpStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oHttpMsg);
                }
            }
        }
            break;
        case CODEC_RESP:
        {
            RedisMsg oRedisMsg;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<RedisMsg>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->AddEvent(pSpecChannel->MutableWatcher()->MutableAsyncWatcher(), Dispatcher::AsyncCallback);
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oRedisMsg))
            {
                if (gc_uiCmdReq & uiFlags)
                {
                    IO<RedisCmd>::OnRequest(pDispatcher, pChannel, (int32)CMD_REQ_REDIS_PROXY, oRedisMsg);
                }
                else
                {
                    IO<RedisStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oRedisMsg);
                }
            }
        }
            break;
        case CODEC_CASS:
        {
            CassResponse oCassResponse;
            auto pSpecChannel = std::static_pointer_cast<SpecChannel<CassResponse>>(pChannel);
            auto pDispatcher = LaborShared::Instance()->GetDispatcher(pSpecChannel->GetOwnerId());
            pDispatcher->AddEvent(pSpecChannel->MutableWatcher()->MutableAsyncWatcher(), Dispatcher::AsyncCallback);
            while (pSpecChannel->Read(uiFlags, uiStepSeq, oCassResponse))
            {
                IO<CassStep>::OnResponse(pDispatcher, pChannel, uiStepSeq, oCassResponse);
            }
        }
            break;
        case CODEC_PRIVATE:
            break;
        case CODEC_UNKNOW:
            break;
        default:
            ;
    }
    return(CODEC_STATUS_OK);
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    oMsgHead.set_len(oMsgBody.ByteSize());
    pChannel->SetStepSeq(uiStepSeq);
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<Cmd>::OnRequest(pDispatcher, pSocketChannel, iCmd, oMsgHead, oMsgBody));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    oMsgHead.set_len(oMsgBody.ByteSize());
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<PbStep>::OnResponse(pDispatcher, pSocketChannel, uiSeq, oMsgHead, oMsgBody));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg)
{
    pChannel->SetStepSeq(uiStepSeq);
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<Module>::OnRequest(pDispatcher, pSocketChannel, oHttpMsg.path(), oHttpMsg));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg)
{
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<HttpStep>::OnResponse(pDispatcher, pSocketChannel, pChannel->GetStepSeq(), oHttpMsg));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg)
{
    pChannel->SetStepSeq(uiStepSeq);
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<RedisCmd>::OnRequest(pDispatcher, pSocketChannel, (int32)CMD_REQ_REDIS_PROXY, oRedisMsg));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg)
{
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<RedisStep>::OnResponse(pDispatcher, pSocketChannel, pChannel->GetStepSeq(), oRedisMsg));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize)
{
    pChannel->SetStepSeq(uiStepSeq);
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<RawCmd>::OnRequest(pDispatcher, pSocketChannel, (int32)CMD_REQ_RAW_DATA, pRaw, uiRawSize));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize)
{
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<RawStep>::OnResponse(pDispatcher, pSocketChannel, pChannel->GetStepSeq(), pRaw, uiRawSize));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const CassMessage& oCassMsg)
{
    auto pSocketChannel = std::static_pointer_cast<SocketChannel>(pChannel);
    pDispatcher->m_pLastActivityChannel = pSocketChannel;
    return(IO<CassStep>::OnResponse(pDispatcher, pSocketChannel, pChannel->GetStepSeq(), oCassMsg));
}

E_CODEC_STATUS CodecFactory::OnNebulaEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart)
{
    E_CODEC_STATUS eCodecStatus;
    int i = iStart;
    for (; ; ++i)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        if (0 == i)
        {
            if (CODEC_PROTO == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecProto>::Recv(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
            else if (CODEC_NEBULA == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecNebula>::Recv(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
            else
            {
                eCodecStatus = IO<CodecNebulaInNode>::Recv(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
        }
        else
        {
            if (CODEC_PROTO == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecProto>::Fetch(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
            else if (CODEC_NEBULA == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecNebula>::Fetch(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
            else
            {
                eCodecStatus = IO<CodecNebulaInNode>::Fetch(pDispatcher, pChannel, oMsgHead, oMsgBody);
            }
        }

        if (CODEC_STATUS_OK == eCodecStatus)
        {
            IO<CodecProto>::OnMessage(pDispatcher, pChannel, oMsgHead, oMsgBody);
        }
        else
        {
            break;
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecFactory::OnHttpEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart)
{
    E_CODEC_STATUS eCodecStatus;
    int i = iStart;
    for (; ; ++i)
    {
        HttpMsg oHttpMsg;
        if (0 == i)
        {
            if (CODEC_HTTP == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecHttp>::Recv(pDispatcher, pChannel, oHttpMsg);
            }
            else
            {
                eCodecStatus = IO<CodecHttp2>::Recv(pDispatcher, pChannel, oHttpMsg);
            }
        }
        else
        {
            if (CODEC_HTTP == pChannel->GetCodecType())
            {
                eCodecStatus = IO<CodecHttp>::Fetch(pDispatcher, pChannel, oHttpMsg);
            }
            else
            {
                eCodecStatus = IO<CodecHttp2>::Fetch(pDispatcher, pChannel, oHttpMsg);
            }
        }

        if (CODEC_STATUS_OK == eCodecStatus
                || CODEC_STATUS_PART_OK == eCodecStatus)
        {
            if (oHttpMsg.http_major() > 1)
            {
                if (oHttpMsg.stream_id() > 0)
                {
                    if (HTTP_REQUEST == oHttpMsg.type())
                    {
                        bool bRes = IO<Module>::OnRequest(pDispatcher, pChannel, oHttpMsg.path(), oHttpMsg);
                        if (!bRes)
                        {
                            HttpMsg oOutHttpMsg;
                            oOutHttpMsg.set_type(HTTP_RESPONSE);
                            oOutHttpMsg.set_status_code(404);
                            oOutHttpMsg.set_http_major(oHttpMsg.http_major());
                            oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
                            IO<CodecHttp2>::SendRequest(pDispatcher, 0, pChannel, oOutHttpMsg);
                        }
                    }
                    else
                    {
                        IO<HttpStep>::OnResponse(pDispatcher, pChannel, oHttpMsg.stream_id(), eCodecStatus, oHttpMsg);
                    }
                }
            }
            else
            {
                if (HTTP_REQUEST == oHttpMsg.type())
                {
                    bool bRes = IO<Module>::OnRequest(pDispatcher, pChannel, oHttpMsg.path(), oHttpMsg);
                    if (!bRes)
                    {
                        HttpMsg oOutHttpMsg;
                        oOutHttpMsg.set_type(HTTP_RESPONSE);
                        oOutHttpMsg.set_status_code(404);
                        oOutHttpMsg.set_http_major(oHttpMsg.http_major());
                        oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
                        IO<CodecHttp>::SendRequest(pDispatcher, 0, pChannel, oOutHttpMsg);
                    }
                }
                else
                {
                    IO<HttpStep>::OnResponse(pDispatcher, pChannel, oHttpMsg.stream_id(), eCodecStatus, oHttpMsg);
                }
            }
        }
        else if (CODEC_STATUS_EOF == eCodecStatus && oHttpMsg.ByteSize() > 10) // http1.0 client close
        {
            IO<HttpStep>::OnResponse(pDispatcher, pChannel, oHttpMsg.stream_id(), eCodecStatus, oHttpMsg);
        }
        else
        {
            break;
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecFactory::OnRedisEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart)
{
    E_CODEC_STATUS eCodecStatus;
    int i = iStart;
    for (; ; ++i)
    {
        RedisMsg oRedisMsg;
        if (0 == i)
        {
            eCodecStatus = IO<CodecResp>::Recv(pDispatcher, pChannel, oRedisMsg);
        }
        else
        {
            eCodecStatus = IO<CodecResp>::Fetch(pDispatcher, pChannel, oRedisMsg);
        }

        if (CODEC_STATUS_OK == eCodecStatus)
        {
            if (pChannel->IsClient())
            {
                IO<RedisStep>::OnResponse(pDispatcher, pChannel, 0, eCodecStatus, oRedisMsg);
            }
            else
            {
                IO<RedisCmd>::OnRequest(pDispatcher, pChannel, (int32)CMD_REQ_REDIS_PROXY, oRedisMsg);
            }
        }
        else
        {
            break;
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS CodecFactory::OnCassEvent(Dispatcher* pDispatcher, std::shared_ptr<SocketChannel> pChannel, int iStart)
{
    E_CODEC_STATUS eCodecStatus;
    int i = iStart;
    for (; ; ++i)
    {
        CassResponse oCassResponse;
        if (0 == i)
        {
            eCodecStatus = IO<CodecCass>::Recv(pDispatcher, pChannel, oCassResponse);
        }
        else
        {
            eCodecStatus = IO<CodecCass>::Fetch(pDispatcher, pChannel, oCassResponse);
        }

        if (CODEC_STATUS_OK == eCodecStatus)
        {
            if (CASS_OP_RESULT != oCassResponse.GetOpcode() && CASS_OP_ERROR != oCassResponse.GetOpcode())
            {
                continue;
            }
            IO<CassStep>::OnResponse(pDispatcher, pChannel, oCassResponse.GetStreamId(), eCodecStatus, oCassResponse);
        }
        else
        {
            break;
        }
    }
    return(eCodecStatus);
}

bool CodecFactory::AutoSwitchCodec(Dispatcher* pDispatcher,
        std::shared_ptr<SocketChannel> pChannel, E_CODEC_TYPE eOriginCodecType, uint32& uiLastCodecPos)
{
    for (uint32 i = uiLastCodecPos + 1; i < s_vecAutoSwitchCodec.size(); ++i)
    {
        if (eOriginCodecType == s_vecAutoSwitchCodec[i])
        {
            continue;
        }

        LOG4_TRACE_DISPATCH("to codec type %d", s_vecAutoSwitchCodec[i]);
        Codec* pCodec = CreateCodec(pDispatcher->GetLogger(), s_vecAutoSwitchCodec[i], pChannel);
        if (pCodec == nullptr)
        {
            LOG4_TRACE_DISPATCH("failed to new codec with codec type %d", s_vecAutoSwitchCodec[i]);
            continue;
        }
        uiLastCodecPos = i;
        std::static_pointer_cast<SocketChannelImpl<CodecNebula>>(pChannel->m_pImpl)->SetCodec(pCodec);
        return(true);
    }
    return(false);
}

} /* namespace neb */

