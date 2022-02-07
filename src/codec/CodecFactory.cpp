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

Codec* CodecFactory::Create(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, std::shared_ptr<SocketChannel> pBindChannel)
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

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    oMsgHead.set_len(oMsgBody.ByteSize());
    pChannel->SetStepSeq(uiStepSeq);
    return(IO<Cmd>::OnRequest(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), iCmd, oMsgHead, oMsgBody));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    MsgHead oMsgHead;
    oMsgHead.set_cmd(iCmd);
    oMsgHead.set_seq(uiSeq);
    oMsgHead.set_len(oMsgBody.ByteSize());
    return(IO<PbStep>::OnResponse(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), uiSeq, oMsgHead, oMsgBody));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg)
{
    pChannel->SetStepSeq(uiStepSeq);
    return(IO<Module>::OnRequest(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), oHttpMsg.path(), oHttpMsg));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const HttpMsg& oHttpMsg)
{
    return(IO<HttpStep>::OnResponse(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), pChannel->GetStepSeq(), oHttpMsg));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg)
{
    pChannel->SetStepSeq(uiStepSeq);
    return(IO<RedisCmd>::OnRequest(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), (int32)CMD_REQ_REDIS_PROXY, oRedisMsg));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const RedisMsg& oRedisMsg)
{
    return(IO<RedisStep>::OnResponse(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), pChannel->GetStepSeq(), oRedisMsg));
}

bool CodecFactory::OnSelfRequest(Dispatcher* pDispatcher, uint32 uiStepSeq, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize)
{
    pChannel->SetStepSeq(uiStepSeq);
    return(IO<RawCmd>::OnRequest(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), (int32)CMD_REQ_RAW_DATA, pRaw, uiRawSize));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const char* pRaw, uint32 uiRawSize)
{
    return(IO<RawStep>::OnResponse(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), pChannel->GetStepSeq(), pRaw, uiRawSize));
}

bool CodecFactory::OnSelfResponse(Dispatcher* pDispatcher, std::shared_ptr<SelfChannel> pChannel, const CassMessage& oCassMsg)
{
    return(IO<CassStep>::OnResponse(pDispatcher, std::static_pointer_cast<SocketChannel>(pChannel), pChannel->GetStepSeq(), oCassMsg));
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

        Codec* pCodec = Create(pDispatcher->GetLogger(), s_vecAutoSwitchCodec[i], pChannel);
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

