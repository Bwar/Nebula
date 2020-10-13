/*******************************************************************************
 * Project:  Nebula
 * @file     Http2Frame.cpp
 * @brief    
 * @author   nebim
 * @date:    2020-05-02
 * @note     
 * Modify history:
 ******************************************************************************/
#include "Http2Frame.hpp"
#include "CodecHttp2.hpp"

namespace neb
{

Http2Frame::Http2Frame(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
    : Codec(pLogger, eCodecType)
{
}

Http2Frame::~Http2Frame()
{
}

E_CODEC_STATUS Http2Frame::Encode(CodecHttp2* pCodecH2,
        const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::Decode(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    switch (stFrameHead.ucType)
    {
        case H2_FRAME_DATA:
            return(DecodeData(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_HEADERS:
            return(DecodeHeaders(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_PRIORITY:
            return(DecodePriority(pCodecH2, stFrameHead, pBuff, pReactBuff));
        case H2_FRAME_RST_STREAM:
            return(DecodeRstStream(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_SETTINGS:
            return(DecodeSetting(pCodecH2, stFrameHead, pBuff, pReactBuff));
        case H2_FRAME_PUSH_PROMISE:
            return(DecodePushPromise(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_PING:
            return(DecodePing(pCodecH2, stFrameHead, pBuff, pReactBuff));
        case H2_FRAME_GOAWAY:
            return(DecodeGoaway(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_WINDOW_UPDATE:
            return(DecodeWindowUpdate(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        case H2_FRAME_CONTINUATION:
            return(DecodeContinuation(pCodecH2, stFrameHead, pBuff, oHttpMsg, pReactBuff));
        default:
            LOG4_ERROR("unknow frame type %d!", stFrameHead.ucType);
            EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                    " unspecific protocol error. This error is for use when a more"
                    " specific error code is not available.", pReactBuff);
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            return(CODEC_STATUS_PART_ERR);
    }
}

E_CODEC_STATUS Http2Frame::DecodeData(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (H2_FRAME_FLAG_PADDED & stFrameHead.ucFlag)
    {
        uint8 ucPadLength = 0;
        pBuff->Read(&ucPadLength, 1);
        uint32 uiDataLength = stFrameHead.uiLength - 1 - ucPadLength;
        if (ucPadLength >= stFrameHead.uiLength - 1)
        {
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                    "detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.",
                    pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        oHttpMsg.mutable_body()->append(pBuff->GetRawReadBuffer(), uiDataLength);
        oHttpMsg.set_data_frame_padding(pBuff->GetRawReadBuffer() + uiDataLength, ucPadLength);
        pBuff->AdvanceReadIndex(stFrameHead.uiLength - 1);
    }
    else
    {
        oHttpMsg.mutable_body()->append(pBuff->GetRawReadBuffer(), stFrameHead.uiLength);
        pBuff->AdvanceReadIndex(stFrameHead.uiLength);
    }
    return(CODEC_STATUS_PART_OK);
}

E_CODEC_STATUS Http2Frame::DecodeHeaders(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    uint8 ucPadLength = 0;
    uint32 uiHeaderLength = stFrameHead.uiLength;
    uint32 uiHeaderBlockEndPos = pBuff->GetReadIndex() + uiHeaderLength;
    if (H2_FRAME_FLAG_PADDED & stFrameHead.ucFlag)
    {
        pBuff->Read(&ucPadLength, 1);
        if (ucPadLength >= stFrameHead.uiLength - 1)
        {
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                    "detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        uiHeaderLength = stFrameHead.uiLength - 1 - ucPadLength;
        uiHeaderBlockEndPos = pBuff->GetReadIndex() + uiHeaderLength;
        oHttpMsg.set_headers_frame_padding(pBuff->GetRawReadBuffer() + uiHeaderLength, ucPadLength);
    }
    if (H2_FRAME_FLAG_PRIORITY & stFrameHead.ucFlag)
    {
        tagPriority stPriority;
        pBuff->Read(&stPriority.uiDependency, 4);
        stPriority.E = (stPriority.uiDependency & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
        stPriority.uiDependency = ntohl(stPriority.uiDependency & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
        pBuff->Read(&stPriority.ucWeight, 1);
        pCodecH2->SetPriority(stFrameHead.uiStreamIdentifier, stPriority);
    }
    E_CODEC_STATUS eCodecStatus;
    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
    {
        eCodecStatus = pCodecH2->UnpackHeader(uiHeaderBlockEndPos, pBuff, oHttpMsg);
    }
    else
    {
        oHttpMsg.mutable_hpack_data()->append(pBuff->GetRawReadBuffer(), uiHeaderLength);
        pBuff->AdvanceReadIndex(stFrameHead.uiLength);
    }
    return(eCodecStatus);
}

E_CODEC_STATUS Http2Frame::DecodePriority(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (stFrameHead.uiLength != 5)
    {
        pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        return(CODEC_STATUS_ERR);
    }
    tagPriority stPriority;
    pBuff->Read(&stPriority.uiDependency, 4);
    stPriority.E = (stPriority.uiDependency & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    stPriority.uiDependency = ntohl(stPriority.uiDependency & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    pBuff->Read(&stPriority.ucWeight, 1);
    if (stPriority.uiDependency == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    pCodecH2->SetPriority(stFrameHead.uiStreamIdentifier, stPriority);
    return(CODEC_STATUS_PART_OK);
}

E_CODEC_STATUS Http2Frame::DecodeRstStream(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (stFrameHead.uiLength != 4)
    {
        pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_FRAME_SIZE_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    int32 iErrCode = 0;
    pBuff->Read(&iErrCode, 4);    // the error code should be H2_ERR_CANCEL
    iErrCode = ntohl(iErrCode);
    pCodecH2->SetErrno(iErrCode);
    pCodecH2->RstStream(stFrameHead.uiStreamIdentifier);
    return(CODEC_STATUS_PART_ERR);
}

E_CODEC_STATUS Http2Frame::DecodeSetting(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier != 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }

    if (H2_FRAME_FLAG_ACK & stFrameHead.ucFlag)
    {
        if (stFrameHead.uiLength != 0)
        {
            pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_FRAME_SIZE_ERROR, "The endpoint detected an"
                    " unspecific protocol error. This error is for use when a more"
                    " specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        return(CODEC_STATUS_OK);
    }
    else
    {
        if (stFrameHead.uiLength % 6 != 0)
        {
            pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_FRAME_SIZE_ERROR, "The endpoint detected an"
                    " unspecific protocol error. This error is for use when a more"
                    " specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }

        uint32 uiSettingNum = stFrameHead.uiLength / 6;
        tagSetting stSetting;
        std::vector<tagSetting> vecSetting;
        for (uint32 i = 0; i < uiSettingNum; ++i)
        {
            pBuff->Read(&stSetting.unIdentifier, 2);
            pBuff->Read(&stSetting.uiValue, 4);
            vecSetting.push_back(stSetting);
        }
        E_H2_ERR_CODE eSettingResult = pCodecH2->Setting(vecSetting);
        if (eSettingResult == H2_ERR_NO_ERROR)
        {
            EncodeSetting(pCodecH2, pReactBuff);
            return(CODEC_STATUS_OK);
        }
        pCodecH2->SetErrno(eSettingResult);
        EncodeGoaway(pCodecH2, eSettingResult, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS Http2Frame::DecodePushPromise(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    uint8 ucPadLength = 0;
    uint32 uiHeaderLength = stFrameHead.uiLength;
    uint32 uiHeaderBlockEndPos = pBuff->GetReadIndex() + uiHeaderLength;
    if (H2_FRAME_FLAG_PADDED & stFrameHead.ucFlag)
    {
        pBuff->Read(&ucPadLength, 1);
        if (ucPadLength >= stFrameHead.uiLength - 1)
        {
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint "
                    "detected an unspecific protocol error. This error is for "
                    "use when a more specific error code is not available.", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        uiHeaderLength = stFrameHead.uiLength - 1 - ucPadLength;
        uiHeaderBlockEndPos = pBuff->GetReadIndex() + uiHeaderLength;
        oHttpMsg.set_push_promise_frame_padding(pBuff->GetRawReadBuffer() + uiHeaderLength, ucPadLength);
    }
    unsigned char R;
    uint32 uiStreamId = 0;
    pBuff->Read(&uiStreamId, 4);
    R = (uiStreamId & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    uiStreamId = ntohl(uiStreamId & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    E_CODEC_STATUS eCodecStatus = pCodecH2->PromiseStream(uiStreamId, pReactBuff);
    if (eCodecStatus == CODEC_STATUS_PART_OK)
    {
        if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
        {
            eCodecStatus = pCodecH2->UnpackHeader(uiHeaderBlockEndPos, pBuff, oHttpMsg);
        }
        else
        {
            oHttpMsg.mutable_hpack_data()->append(pBuff->GetRawReadBuffer(), uiHeaderLength);
            pBuff->AdvanceReadIndex(stFrameHead.uiLength);
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS Http2Frame::DecodePing(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier != 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "TYPE_PING streamId != 0", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (stFrameHead.uiLength != 8)
    {
        pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_FRAME_SIZE_ERROR, "TYPE_PING length != 8", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    int iPayload1 = 0;
    int iPayload2 = 0;
    pBuff->Read(&iPayload1, 4);
    pBuff->Read(&iPayload2, 4);
    if (!(stFrameHead.ucFlag & H2_FRAME_FLAG_ACK))
    {
        EncodePing(pCodecH2, true, iPayload1, iPayload2, pReactBuff);
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::DecodeGoaway(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier != 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    unsigned char R;
    uint32 uiLastStreamId = 0;
    int32 iErrCode = 0;
    std::string strDebugData;
    pBuff->Read(&uiLastStreamId, 4);
    pBuff->Read(&iErrCode, 4);
    R = (uiLastStreamId & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    uiLastStreamId = ntohl(uiLastStreamId & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    iErrCode = ntohl(iErrCode);
    strDebugData.assign(pBuff->GetRawReadBuffer(), stFrameHead.uiLength - 8);
    pBuff->AdvanceReadIndex(stFrameHead.uiLength - 8);
    pCodecH2->SetGoaway(uiLastStreamId);
    LOG4_WARNING("errcode %d: %s", iErrCode, strDebugData.c_str());
    pCodecH2->SetErrno(iErrCode);
    return(CODEC_STATUS_PART_ERR);
}

E_CODEC_STATUS Http2Frame::DecodeWindowUpdate(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiLength != 4)
    {
        pCodecH2->SetErrno(H2_ERR_FRAME_SIZE_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_FRAME_SIZE_ERROR, "TYPE_WINDOW_UPDATE length != 4", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    uint32 uiIncrement = 0;
    pBuff->Read(&uiIncrement, 4);
    uiIncrement &= H2_DATA_MASK_4_BYTE_LOW_31_BIT;
    if (uiIncrement == 0)
    {
        if (stFrameHead.uiStreamIdentifier == 0)
        {
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "TYPE_WINDOW_UPDATE length == 0 on connection", pReactBuff);
            return(CODEC_STATUS_ERR);
        }
        else
        {
            pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
            EncodeRstStream(pCodecH2, stFrameHead.uiStreamIdentifier, H2_ERR_PROTOCOL_ERROR, pReactBuff);
            return(CODEC_STATUS_PART_ERR);
        }
    }
    else
    {
        pCodecH2->WindowUpdate(stFrameHead.uiStreamIdentifier, uiIncrement);
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::DecodeContinuation(CodecHttp2* pCodecH2,
        const tagH2FrameHead& stFrameHead, CBuffer* pBuff,
        HttpMsg& oHttpMsg, CBuffer* pReactBuff)
{
    if (stFrameHead.uiStreamIdentifier == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        EncodeGoaway(pCodecH2, H2_ERR_PROTOCOL_ERROR, "The endpoint detected an"
                " unspecific protocol error. This error is for use when a more"
                " specific error code is not available.", pReactBuff);
        return(CODEC_STATUS_ERR);
    }
    if (H2_FRAME_FLAG_END_HEADERS & stFrameHead.ucFlag)
    {
        CBuffer oBuffer;
        oHttpMsg.mutable_hpack_data()->append(pBuff->GetRawReadBuffer(), stFrameHead.uiLength);
        pBuff->AdvanceReadIndex(stFrameHead.uiLength);
        oBuffer.Write(oHttpMsg.hpack_data().c_str(), oHttpMsg.hpack_data().size());
        oHttpMsg.mutable_hpack_data()->clear();
        E_CODEC_STATUS eCodecStatus = pCodecH2->UnpackHeader(oBuffer.GetReadIndex() + oBuffer.ReadableBytes(), &oBuffer, oHttpMsg);
        return(eCodecStatus);
    }
    else
    {
        oHttpMsg.mutable_hpack_data()->append(pBuff->GetRawReadBuffer(), stFrameHead.uiLength);
        pBuff->AdvanceReadIndex(stFrameHead.uiLength);
        return(CODEC_STATUS_PAUSE);
    }
}

void Http2Frame::EncodeFrameHeader(const tagH2FrameHead& stFrameHead, CBuffer* pBuff)
{
    uint32 uiIdentifier = stFrameHead.cR;
    uiIdentifier <<= 31;
    uiIdentifier |= (htonl(stFrameHead.uiStreamIdentifier));
    pBuff->Write(&htonl(stFrameHead.uiLength), 3);
    pBuff->Write(&stFrameHead.ucType, 1);
    pBuff->Write(&stFrameHead.ucFlag, 1);
    pBuff->Write(&uiIdentifier, 4);
}

void Http2Frame::EncodePriority(const tagPriority& stPriority, CBuffer* pBuff)
{
    if (stPriority.uiDependency > 0)
    {
        uint32 uiPriority = stPriority.E;
        uiPriority <<= 31;
        uiPriority |= (htonl(stPriority.uiDependency));
        pBuff->Write(&uiPriority, 4);
        pBuff->Write(&stPriority.ucWeight, 1);
    }
}

E_CODEC_STATUS Http2Frame::EncodeData(CodecHttp2* pCodecH2,
        uint32 uiStreamId, const HttpMsg& oHttpMsg,
        const std::string& strPadding, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_DATA;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    if (strPadding.size() > 0)
    {
        stFrameHead.uiLength = 1 + oHttpMsg.body().size() + strPadding.size();
        stFrameHead.ucFlag |= H2_FRAME_FLAG_PADDED;
        EncodeFrameHeader(stFrameHead, pBuff);
        pBuff->Write(&(htonl(strPadding.size())), 1);
        pBuff->Write(oHttpMsg.body().c_str(), oHttpMsg.body().size());
        pBuff->Write(strPadding.c_str(), strPadding.size());
    }
    else
    {
        stFrameHead.uiLength = oHttpMsg.body().size();
        EncodeFrameHeader(stFrameHead, pBuff);
        pBuff->Write(oHttpMsg.body().c_str(), oHttpMsg.body().size());
    }
    return(CODEC_STATUS_PART_OK);
}

E_CODEC_STATUS Http2Frame::EncodeHeaders(CodecHttp2* pCodecH2,
        uint32 uiStreamId, const HttpMsg& oHttpMsg,
        const tagPriority& stPriority, const std::string& strPadding, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    tagH2FrameHead stFrameHead;
    CBuffer oBuffer;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_HEADERS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    pCodecH2->PackHeader(oHttpMsg, &oBuffer);
    if (strPadding.size() > 0)
    {
        uint32 uiAddtionLength = 0;
        stFrameHead.ucFlag |= H2_FRAME_FLAG_PADDED;
        if (stPriority.uiDependency > 0)
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_PRIORITY;
            // 6bytes = 48bits = 8 + 1 + 31 + 8
            stFrameHead.uiLength = 6 + oBuffer.ReadableBytes() + strPadding.size();
            uiAddtionLength = 6 + strPadding.size();
        }
        else
        {
            stFrameHead.uiLength = 1 + oBuffer.ReadableBytes() + strPadding.size();
            uiAddtionLength = 1 + strPadding.size();
        }

        if (stFrameHead.uiLength > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(&(htonl(strPadding.size())), 1);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            return(EncodeContinuation(pCodecH2, uiStreamId, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(&(htonl(strPadding.size())), 1);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            return(CODEC_STATUS_OK);
        }
    }
    else
    {
        uint32 uiAddtionLength = 0;
        if (stPriority.uiDependency > 0)
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_PRIORITY;
            // 5bytes = 40bits = 1 + 31 + 8
            stFrameHead.uiLength = 5 + oBuffer.ReadableBytes();
            uiAddtionLength = 5;
        }
        else
        {
            stFrameHead.uiLength = oBuffer.ReadableBytes();
        }

        if (stFrameHead.uiLength > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
            EncodeFrameHeader(stFrameHead, pBuff);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            return(EncodeContinuation(pCodecH2, uiStreamId, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            EncodeFrameHeader(stFrameHead, pBuff);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            return(CODEC_STATUS_OK);
        }
    }
}

E_CODEC_STATUS Http2Frame::EncodePriority(CodecHttp2* pCodecH2,
        uint32 uiStreamId, const tagPriority& stPriority, CBuffer* pBuff)
{
    if (stPriority.uiDependency == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_HEADERS;
    stFrameHead.ucFlag = H2_FRAME_FLAG_PRIORITY;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    stFrameHead.uiLength = 5;
    uint32 uiPriority = stPriority.E;
    uiPriority <<= 31;
    uiPriority |= (htonl(stPriority.uiDependency));
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&uiPriority, 4);
    pBuff->Write(&stPriority.ucWeight, 1);
    return(CODEC_STATUS_PART_OK);
}

E_CODEC_STATUS Http2Frame::EncodeRstStream(CodecHttp2* pCodecH2,
        uint32 uiStreamId, int32 iErrCode, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_RST_STREAM;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    stFrameHead.uiLength = 4;
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&(htonl(iErrCode)), 4);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeSetting(CodecHttp2* pCodecH2,
        const std::vector<tagSetting>& vecSetting, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_RST_STREAM;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 6 * vecSetting.size();
    if (stFrameHead.uiLength == 0)  // H2_FRAME_FLAG_ACK
    {
        stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    }
    EncodeFrameHeader(stFrameHead, pBuff);
    for (uint32 i = 0; i < vecSetting.size(); ++i)
    {
        pBuff->Write(&vecSetting[i].unIdentifier, 2);
        pBuff->Write(&vecSetting[i].uiValue, 4);
    }
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeSetting(CodecHttp2* pCodecH2, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_RST_STREAM;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 0;
    stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    EncodeFrameHeader(stFrameHead, pBuff);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodePushPromise(CodecHttp2* pCodecH2,
        uint32 uiStreamId, uint32 uiPromiseStreamId,
        const std::string& strPadding, const HttpMsg& oHttpMsg, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_ERR);
    }

    tagH2FrameHead stFrameHead;
    CBuffer oBuffer;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_HEADERS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    pCodecH2->PackHeader(oHttpMsg, &oBuffer);
    uint32 uiAddtionLength = 0;
    if (strPadding.size() > 0)
    {
        stFrameHead.ucFlag |= H2_FRAME_FLAG_PADDED;
        stFrameHead.uiLength = 5 + oBuffer.ReadableBytes() + strPadding.size();
        uiAddtionLength = 5 + strPadding.size();
        if (stFrameHead.uiLength > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(&(htonl(strPadding.size())), 1);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            return(EncodeContinuation(pCodecH2, uiStreamId, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(&(htonl(strPadding.size())), 1);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            return(CODEC_STATUS_OK);
        }
    }
    else
    {
        stFrameHead.uiLength = 4 + oBuffer.ReadableBytes();
        uiAddtionLength = 4;
        if (stFrameHead.uiLength > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
            EncodeFrameHeader(stFrameHead, pBuff);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            return(EncodeContinuation(pCodecH2, uiStreamId, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            EncodeFrameHeader(stFrameHead, pBuff);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            return(CODEC_STATUS_OK);
        }
    }
}

E_CODEC_STATUS Http2Frame::EncodePing(CodecHttp2* pCodecH2,
        bool bAck, int32 iPayload1, int32 iPayload2, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_PING;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 8;
    if (bAck)  // H2_FRAME_FLAG_ACK
    {
        stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    }
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&iPayload1, 4);
    pBuff->Write(&iPayload2, 4);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeGoaway(CodecHttp2* pCodecH2,
        int32 iErrCode, const std::string& strDebugData, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_GOAWAY;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0x0;
    stFrameHead.uiLength = 8 + strDebugData.size();
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&(htonl(pCodecH2->GetLastStreamId())), 4);
    pBuff->Write(&(htonl(iErrCode)), 4);
    pBuff->Write(strDebugData.c_str(), strDebugData.size());
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeWindowUpdate(CodecHttp2* pCodecH2,
        uint32 uiStreamId, uint32 uiIncrement, CBuffer* pBuff)
{
    if (uiIncrement == 0 || uiIncrement > H2_DATA_MASK_4_BYTE_LOW_31_BIT)
    {
        return(CODEC_STATUS_PART_ERR);
    }
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_WINDOW_UPDATE;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    stFrameHead.uiLength = 4;
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&uiIncrement, 4);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeContinuation(CodecHttp2* pCodecH2,
        uint32 uiStreamId, CBuffer* pHpackBuff, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_ERR);
    }
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_CONTINUATION;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    while (pHpackBuff->ReadableBytes() > 0)
    {
        if (pHpackBuff->ReadableBytes() > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
        }
        else
        {
            stFrameHead.uiLength = pHpackBuff->ReadableBytes();
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
        }
        EncodeFrameHeader(stFrameHead, pBuff);
        pBuff->Write(pHpackBuff->GetRawBuffer(), stFrameHead.uiLength);
        pHpackBuff->AdvanceReadIndex(stFrameHead.uiLength);
    }
    return(CODEC_STATUS_OK);
}

} /* namespace neb */

