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
#include "Http2Stream.hpp"
#include "codec/CodecUtil.hpp"

namespace neb
{

Http2Frame::Http2Frame(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, Http2Stream* pStream)
    : Codec(pLogger, eCodecType), m_pStream(pStream)
{
}

Http2Frame::~Http2Frame()
{
    m_pStream = nullptr;
    for (auto iter = m_listWaittingFrameData.begin();
            iter != m_listWaittingFrameData.end(); ++iter)
    {
        DELETE(*iter);
    }
    m_listWaittingFrameData.clear();
    LOG4_TRACE("codec type %d", GetCodecType());
}

void Http2Frame::WriteMediumInt(uint32 uiValue, CBuffer* pBuff)
{
    char szData[3] = {0};
    szData[0] = (uiValue >> 16) & 0xFF;
    szData[1] = (uiValue >> 8) & 0xFF;
    szData[2] = uiValue & 0xFF;
    pBuff->Write(szData, 3);
}

void Http2Frame::ReadMediumInt(CBuffer* pBuff, uint32& uiValue)
{
    char szData[3] = {0};
    pBuff->ReadByte(szData[0]);
    pBuff->ReadByte(szData[1]);
    pBuff->ReadByte(szData[2]);
    uiValue = (((uint32)szData[0] & 0xFF) << 16);
    uiValue |= (((uint32)szData[1] & 0xFF) << 8);
    uiValue |= (uint32)szData[2] & 0xFF;
}

void Http2Frame::DecodeFrameHeader(CBuffer* pBuff, tagH2FrameHead& stFrameHead)
{
    ReadMediumInt(pBuff, stFrameHead.uiLength);
    pBuff->Read(&stFrameHead.ucType, 1);
    pBuff->Read(&stFrameHead.ucFlag, 1);
    pBuff->Read(&stFrameHead.uiStreamIdentifier, 4);
    stFrameHead.cR = (stFrameHead.uiStreamIdentifier & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    stFrameHead.uiStreamIdentifier = CodecUtil::N2H(stFrameHead.uiStreamIdentifier & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
}

void Http2Frame::EncodeFrameHeader(const tagH2FrameHead& stFrameHead, CBuffer* pBuff)
{
    uint32 uiIdentifier = stFrameHead.cR;
    uiIdentifier <<= 31;
    uiIdentifier |= (CodecUtil::H2N(stFrameHead.uiStreamIdentifier));
    WriteMediumInt(stFrameHead.uiLength, pBuff);
    pBuff->Write(&stFrameHead.ucType, 1);
    pBuff->Write(&stFrameHead.ucFlag, 1);
    pBuff->Write(&uiIdentifier, 4);
}

E_CODEC_STATUS Http2Frame::EncodeSetting(const std::vector<tagSetting>& vecSetting, std::string& strSettingFrame)
{
    uint16 unIdentifier = 0;
    uint32 uiValue = 0;
    CBuffer oBuff;
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_SETTINGS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 6 * vecSetting.size();
    if (stFrameHead.uiLength == 0)  // H2_FRAME_FLAG_ACK
    {
        stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    }
    EncodeFrameHeader(stFrameHead, &oBuff);
    for (uint32 i = 0; i < vecSetting.size(); ++i)
    {
        unIdentifier = CodecUtil::H2N(vecSetting[i].unIdentifier);
        uiValue = CodecUtil::H2N(vecSetting[i].uiValue);
        oBuff.Write(&unIdentifier, 2);
        oBuff.Write(&uiValue, 4);
    }
    strSettingFrame.assign(oBuff.GetRawReadBuffer(), oBuff.ReadableBytes());
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::Encode(CodecHttp2* pCodecH2,
        const HttpMsg& oHttpMsg, const tagPriority& stPriority,
        const std::string& strPadding, CBuffer* pBuff)
{
    bool bEndStream = false;
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    if (oHttpMsg.headers_size() > 0)
    {
        if (oHttpMsg.body().size() == 0)
        {
            bEndStream = true;
        }
        eCodecStatus = EncodeHeaders(pCodecH2, oHttpMsg.stream_id(), oHttpMsg, stPriority, strPadding, bEndStream, pBuff);
        if (CODEC_STATUS_PART_ERR == eCodecStatus
                || CODEC_STATUS_ERR == eCodecStatus)
        {
            return(eCodecStatus);
        }
    }
    if (oHttpMsg.body().size() > 0)
    {
        if (oHttpMsg.trailer_header_size() == 0)
        {
            bEndStream = true;
        }
        eCodecStatus = EncodeData(pCodecH2, oHttpMsg.stream_id(), oHttpMsg, bEndStream, strPadding, pBuff);
        if (CODEC_STATUS_PART_ERR == eCodecStatus
                || CODEC_STATUS_ERR == eCodecStatus)
        {
            return(eCodecStatus);
        }
    }
    if (oHttpMsg.trailer_header_size() > 0)
    {
        bEndStream = true;
        eCodecStatus = EncodeHeaders(pCodecH2, oHttpMsg.stream_id(), oHttpMsg, stPriority, strPadding, bEndStream, pBuff);
    }
    return(eCodecStatus);
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
    pCodecH2->ShrinkRecvWindow(stFrameHead.uiStreamIdentifier, stFrameHead.uiLength, pReactBuff);
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
        stPriority.uiDependency = CodecUtil::N2H(stPriority.uiDependency & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
        pBuff->Read(&stPriority.ucWeight, 1);
        LOG4_TRACE("SetPriority(identifier = %u, E = %u, ucWeight = %u, uiDependency = %u)",
                stFrameHead.uiStreamIdentifier, stPriority.E, stPriority.ucWeight, stPriority.uiDependency);
        pCodecH2->SetPriority(stFrameHead.uiStreamIdentifier, stPriority);
    }
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
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
    stPriority.uiDependency = CodecUtil::N2H(stPriority.uiDependency & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
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
    iErrCode = CodecUtil::N2H((uint32)iErrCode);
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
            stSetting.unIdentifier = CodecUtil::N2H(stSetting.unIdentifier);
            stSetting.uiValue = CodecUtil::N2H(stSetting.uiValue);
            vecSetting.push_back(stSetting);
            LOG4_TRACE("stSetting.unIdentifier = %u, stSetting.uiValue = %u", stSetting.unIdentifier, stSetting.uiValue);
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
    //unsigned char R;
    uint32 uiStreamId = 0;
    pBuff->Read(&uiStreamId, 4);
    //R = (uiStreamId & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    uiStreamId = CodecUtil::N2H(uiStreamId & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
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
    //unsigned char R;
    uint32 uiLastStreamId = 0;
    int32 iErrCode = 0;
    std::string strDebugData;
    pBuff->Read(&uiLastStreamId, 4);
    pBuff->Read(&iErrCode, 4);
    //R = (uiLastStreamId & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    uiLastStreamId = CodecUtil::N2H(uiLastStreamId & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    iErrCode = CodecUtil::N2H((uint32)iErrCode);
    strDebugData.assign(pBuff->GetRawReadBuffer(), stFrameHead.uiLength - 8);
    pBuff->AdvanceReadIndex(stFrameHead.uiLength - 8);
    pCodecH2->SetGoaway(uiLastStreamId);
    pCodecH2->SetErrno(iErrCode);
    return(CODEC_STATUS_ERR);
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
    //char cR = 0;
    uint32 uiIncrement = 0;
    pBuff->Read(&uiIncrement, 4);
    //cR = (uiIncrement & H2_DATA_MASK_4_BYTE_HIGHEST_BIT) >> 31;
    uiIncrement = CodecUtil::N2H(stFrameHead.uiStreamIdentifier & H2_DATA_MASK_4_BYTE_LOW_31_BIT);
    if (uiIncrement == 0)
    {
        // TODO just to confirm: curl --http2 receive MAGIC SETTING SETTING_ACK WINDOW_UPDATE(with an flow-control window increment of 0)
        if (pCodecH2->GetLastStreamId() == 0)
        {
            LOG4_TRACE("upgrade ignore an flow-control window increment of 0.");
        }
        else
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
    }
    else
    {
        pCodecH2->WindowUpdate(stFrameHead.uiStreamIdentifier, uiIncrement);
        if (stFrameHead.uiStreamIdentifier > 0)
        {
            SendWaittingFrameData(pCodecH2, pReactBuff);
        }
        else
        {
            pCodecH2->SendWaittingFrameData(pReactBuff);
        }
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

E_CODEC_STATUS Http2Frame::EncodeData(CodecHttp2* pCodecH2,
        uint32 uiStreamId, const HttpMsg& oHttpMsg, bool bEndStream,
        const std::string& strPadding, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    E_CODEC_STATUS eEncodeStatus = CODEC_STATUS_OK;
    const char* pBodyData = oHttpMsg.body().c_str();
    uint32 uiDataLen = oHttpMsg.body().size();
    uint32 uiEncodedDataLen = 0;
    while (uiEncodedDataLen < uiDataLen)
    {
        eEncodeStatus = EncodeData(pCodecH2, uiStreamId, pBodyData, uiDataLen, bEndStream, strPadding, uiEncodedDataLen, pBuff);
        if (eEncodeStatus != CODEC_STATUS_OK)
        {
            return(eEncodeStatus);
        }
        pBodyData += uiEncodedDataLen;
        uiDataLen -= uiEncodedDataLen;
    }
    return(eEncodeStatus);
}

E_CODEC_STATUS Http2Frame::EncodeHeaders(CodecHttp2* pCodecH2,
        uint32 uiStreamId, const HttpMsg& oHttpMsg,
        const tagPriority& stPriority, const std::string& strPadding,
        bool bEndStream, CBuffer* pBuff)
{
    if (uiStreamId == 0x0)
    {
        pCodecH2->SetErrno(H2_ERR_PROTOCOL_ERROR);
        return(CODEC_STATUS_PART_ERR);
    }
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_PART_OK;
    tagH2FrameHead stFrameHead;
    CBuffer oBuffer;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_HEADERS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    if (oHttpMsg.body().size() > 0)
    {
        if (bEndStream)
        {
            pCodecH2->PackHeader(oHttpMsg, H2_HEADER_TRAILER, &oBuffer);
        }
        else
        {
            pCodecH2->PackHeader(oHttpMsg, H2_HEADER_PSEUDO | H2_HEADER_NORMAL, &oBuffer);
        }
    }
    else
    {
        pCodecH2->PackHeader(oHttpMsg, H2_HEADER_PSEUDO | H2_HEADER_NORMAL | H2_HEADER_TRAILER, &oBuffer);
    }
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
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pBuff->Write(&unNetLength, 1);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            EncodeSetStreamState(stFrameHead);
            return(EncodeContinuation(pCodecH2, uiStreamId, bEndStream, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
                eCodecStatus = CODEC_STATUS_OK;
            }
            EncodeFrameHeader(stFrameHead, pBuff);
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pBuff->Write(&unNetLength, 1);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            EncodeSetStreamState(stFrameHead);
            return(eCodecStatus);
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
            EncodeSetStreamState(stFrameHead);
            return(EncodeContinuation(pCodecH2, uiStreamId, bEndStream, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
                eCodecStatus = CODEC_STATUS_OK;
            }
            EncodeFrameHeader(stFrameHead, pBuff);
            EncodePriority(stPriority, pBuff);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            EncodeSetStreamState(stFrameHead);
            LOG4_TRACE("stFrameHead.uiLength = %u, stFrameHead.ucFlag = 0x%X, pBuff->ReadableBytes() = %u",
                    stFrameHead.uiLength, stFrameHead.ucFlag, pBuff->ReadableBytes());
            return(eCodecStatus);
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
    uiPriority |= (CodecUtil::H2N(stPriority.uiDependency));
    EncodeFrameHeader(stFrameHead, pBuff);
    pBuff->Write(&uiPriority, 4);
    pBuff->Write(&stPriority.ucWeight, 1);
    EncodeSetStreamState(stFrameHead);
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
    uint32 uiErrCode = CodecUtil::H2N((uint32)iErrCode);
    pBuff->Write(&uiErrCode, 4);
    EncodeSetStreamState(stFrameHead);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeSetting(CodecHttp2* pCodecH2,
        const std::vector<tagSetting>& vecSetting, CBuffer* pBuff)
{
    uint16 unIdentifier = 0;
    uint32 uiValue = 0;
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_SETTINGS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 6 * vecSetting.size();
    if (stFrameHead.uiLength == 0)  // H2_FRAME_FLAG_ACK
    {
        stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    }
    else
    {
        pCodecH2->Setting(vecSetting);
    }
    EncodeFrameHeader(stFrameHead, pBuff);
    for (uint32 i = 0; i < vecSetting.size(); ++i)
    {
        unIdentifier = CodecUtil::H2N(vecSetting[i].unIdentifier);
        uiValue = CodecUtil::H2N(vecSetting[i].uiValue);
        pBuff->Write(&unIdentifier, 2);
        pBuff->Write(&uiValue, 4);
    }
    EncodeSetStreamState(stFrameHead);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeSetting(CodecHttp2* pCodecH2, CBuffer* pBuff)
{
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_SETTINGS;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = 0;
    stFrameHead.uiLength = 0;
    stFrameHead.ucFlag |= H2_FRAME_FLAG_ACK;
    EncodeFrameHeader(stFrameHead, pBuff);
    EncodeSetStreamState(stFrameHead);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodePushPromise(CodecHttp2* pCodecH2,
        uint32 uiStreamId, uint32 uiPromiseStreamId,
        const std::string& strPadding, const HttpMsg& oHttpMsg,
        bool bEndStream, CBuffer* pBuff)
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
    if (bEndStream && oHttpMsg.body().size() > 0)
    {
        pCodecH2->PackHeader(oHttpMsg, true, &oBuffer);
    }
    else
    {
        pCodecH2->PackHeader(oHttpMsg, false, &oBuffer);
    }
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
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pBuff->Write(&unNetLength, 1);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength - uiAddtionLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            oBuffer.AdvanceReadIndex(stFrameHead.uiLength - uiAddtionLength);
            EncodeSetStreamState(stFrameHead);
            return(EncodeContinuation(pCodecH2, uiStreamId, bEndStream, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
            }
            EncodeFrameHeader(stFrameHead, pBuff);
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pBuff->Write(&unNetLength, 1);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            EncodeSetStreamState(stFrameHead);
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
            EncodeSetStreamState(stFrameHead);
            return(EncodeContinuation(pCodecH2, uiStreamId, bEndStream, &oBuffer, pBuff));
        }
        else
        {
            stFrameHead.ucFlag |= H2_FRAME_FLAG_END_HEADERS;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
            }
            EncodeFrameHeader(stFrameHead, pBuff);
            // ignore R
            pBuff->Write(&uiPromiseStreamId, 4);
            pBuff->Write(oBuffer.GetRawReadBuffer(), stFrameHead.uiLength);
            EncodeSetStreamState(stFrameHead);
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
    EncodeSetStreamState(stFrameHead);
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

    uint32 uiNetStreamId = CodecUtil::H2N(pCodecH2->GetLastStreamId());
    uint32 uiNetErrCode = CodecUtil::H2N((uint32)iErrCode);
    pBuff->Write(&uiNetStreamId, 4);
    pBuff->Write(&uiNetErrCode, 4);
    pBuff->Write(strDebugData.c_str(), strDebugData.size());
    EncodeSetStreamState(stFrameHead);
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
    uiIncrement = CodecUtil::H2N(uiIncrement);
    pBuff->Write(&uiIncrement, 4);
    EncodeSetStreamState(stFrameHead);
    return(CODEC_STATUS_OK);
}

E_CODEC_STATUS Http2Frame::EncodeContinuation(CodecHttp2* pCodecH2,
        uint32 uiStreamId, bool bEndStream, CBuffer* pHpackBuff, CBuffer* pBuff)
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
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
            }
        }
        EncodeFrameHeader(stFrameHead, pBuff);
        pBuff->Write(pHpackBuff->GetRawBuffer(), stFrameHead.uiLength);
        pHpackBuff->AdvanceReadIndex(stFrameHead.uiLength);
    }
    EncodeSetStreamState(stFrameHead);
    return(CODEC_STATUS_OK);
}

void Http2Frame::EncodePriority(const tagPriority& stPriority, CBuffer* pBuff)
{
    if (stPriority.uiDependency > 0)
    {
        uint32 uiPriority = stPriority.E;
        uiPriority <<= 31;
        uiPriority |= (CodecUtil::H2N(stPriority.uiDependency));
        pBuff->Write(&uiPriority, 4);
        pBuff->Write(&stPriority.ucWeight, 1);
    }
}

void Http2Frame::EncodeSetStreamState(const tagH2FrameHead& stFrameHead)
{
    if (m_pStream != nullptr)
    {
        m_pStream->EncodeSetState(stFrameHead);
    }
}

E_CODEC_STATUS Http2Frame::EncodeData(CodecHttp2* pCodecH2, uint32 uiStreamId,
        const char* pData, uint32 uiDataLen, bool bEndStream, const std::string& strPadding,
        uint32& uiEncodedDataLen, CBuffer* pBuff)
{
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_PART_OK;
    tagH2FrameHead stFrameHead;
    stFrameHead.cR = 0;
    stFrameHead.ucType = H2_FRAME_DATA;
    stFrameHead.ucFlag = 0;
    stFrameHead.uiStreamIdentifier = uiStreamId;
    if (strPadding.size() > 0)
    {
        stFrameHead.uiLength = 1 + uiDataLen + strPadding.size();
        if (stFrameHead.uiLength > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
            uiEncodedDataLen = stFrameHead.uiLength - 1 - strPadding.size();
        }
        else
        {
            uiEncodedDataLen = uiDataLen;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
            }
            eCodecStatus = CODEC_STATUS_OK;
        }
        stFrameHead.ucFlag |= H2_FRAME_FLAG_PADDED;
        if (uiEncodedDataLen < pCodecH2->GetSendWindowSize()
                && (int32)uiEncodedDataLen < m_pStream->GetSendWindowSize())
        {
            EncodeFrameHeader(stFrameHead, pBuff);
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pBuff->Write(&unNetLength, 1);
            pBuff->Write(pData, uiEncodedDataLen);
            pBuff->Write(strPadding.c_str(), strPadding.size());
            pCodecH2->ShrinkSendWindow(uiStreamId, uiEncodedDataLen);
            EncodeSetStreamState(stFrameHead);
        }
        else
        {
            CBuffer* pWaittingBuff = nullptr;
            try
            {
                pWaittingBuff = new CBuffer();
            }
            catch(std::bad_alloc& e)
            {
                LOG4_ERROR("%s", e.what());
                return(CODEC_STATUS_PART_ERR);
            }
            m_listWaittingFrameData.push_back(pWaittingBuff);
            EncodeFrameHeader(stFrameHead, pWaittingBuff);
            uint16 unNetLength = CodecUtil::H2N(strPadding.size());
            pWaittingBuff->Write(&unNetLength, 1);
            pWaittingBuff->Write(pData, uiEncodedDataLen);
            pWaittingBuff->Write(strPadding.c_str(), strPadding.size());
            m_stLastDataFrameHead = stFrameHead;
            eCodecStatus = CODEC_STATUS_PART_OK;
        }
    }
    else
    {
        if (uiDataLen > pCodecH2->GetMaxFrameSize())
        {
            stFrameHead.uiLength = pCodecH2->GetMaxFrameSize();
        }
        else
        {
            stFrameHead.uiLength = uiDataLen;
            if (bEndStream)
            {
                stFrameHead.ucFlag |= H2_FRAME_FLAG_END_STREAM;
            }
            eCodecStatus = CODEC_STATUS_OK;
        }
        if (uiEncodedDataLen < pCodecH2->GetSendWindowSize()
                && (int32)uiEncodedDataLen < m_pStream->GetSendWindowSize())
        {
            uiEncodedDataLen = stFrameHead.uiLength;
            EncodeFrameHeader(stFrameHead, pBuff);
            pBuff->Write(pData, uiEncodedDataLen);
            pCodecH2->ShrinkSendWindow(uiStreamId, uiEncodedDataLen);
            EncodeSetStreamState(stFrameHead);
        }
        else
        {
            CBuffer* pWaittingBuff = nullptr;
            try
            {
                pWaittingBuff = new CBuffer();
            }
            catch(std::bad_alloc& e)
            {
                LOG4_ERROR("%s", e.what());
                return(CODEC_STATUS_PART_ERR);
            }
            m_listWaittingFrameData.push_back(pWaittingBuff);
            uiEncodedDataLen = stFrameHead.uiLength;
            EncodeFrameHeader(stFrameHead, pWaittingBuff);
            pWaittingBuff->Write(pData, uiEncodedDataLen);
            m_stLastDataFrameHead = stFrameHead;
            eCodecStatus = CODEC_STATUS_PART_OK;
        }
    }
    return(eCodecStatus);
}

E_CODEC_STATUS Http2Frame::SendWaittingFrameData(CodecHttp2* pCodecH2, CBuffer* pBuff)
{
    LOG4_TRACE("m_listWaittingFrameData.size() = %u", m_listWaittingFrameData.size());
    if (m_listWaittingFrameData.empty())
    {
        return(CODEC_STATUS_OK);
    }
    uint32 uiDataLen = 0;
    for (auto iter = m_listWaittingFrameData.begin();
            iter != m_listWaittingFrameData.end(); ++iter)
    {
        uiDataLen = (*iter)->ReadableBytes();
        if (uiDataLen < pCodecH2->GetSendWindowSize()
                && (int32)uiDataLen < m_pStream->GetSendWindowSize())
        {
            pBuff->Write(*iter, uiDataLen);
            DELETE(*iter);
            m_listWaittingFrameData.erase(iter);
            iter = m_listWaittingFrameData.begin();
        }
        else
        {
            break;
        }
    }
    if (m_listWaittingFrameData.empty())
    {
        EncodeSetStreamState(m_stLastDataFrameHead);
        return(CODEC_STATUS_OK);
    }
    else
    {
        return(CODEC_STATUS_PART_OK);
    }
}

} /* namespace neb */

