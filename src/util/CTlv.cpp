/*******************************************************************************
 * Project:  neb
 * @file     CTlv.cpp
 * @brief 
 * @author   bwarliao
 * @date:    2014年8月21日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CTlv.hpp"

namespace neb
{

CTlv::CTlv()
    : CStreamCodec(STREAM_CODEC_TLV), m_uiType(0), m_uiLength(0)
{
}

CTlv::CTlv(unsigned int uiType, unsigned int uiLength, const CBuffer& oBuff)
    : CStreamCodec(STREAM_CODEC_TLV), m_uiType(uiType), m_uiLength(uiLength)
{
    m_oBuff.Write(oBuff.GetRawReadBuffer(), m_uiLength);
}

CTlv::~CTlv()
{
}

bool CTlv::Encode(unsigned int uiType, unsigned int uiInBuffLength, const CBuffer& oInBuff, CBuffer& oOutBuff)
{
    unsigned int uiWriteLength = 0;
    uiWriteLength = oOutBuff.Write(&uiType, sizeof(uiType));
    if (uiWriteLength != sizeof(uiType))
    {
        return(false);
    }

    uiWriteLength = oOutBuff.Write(&uiInBuffLength, sizeof(uiInBuffLength));
    if (uiWriteLength != sizeof(uiInBuffLength))
    {
        return(false);
    }

    uiWriteLength = oOutBuff.Write(oInBuff.GetRawReadBuffer(), uiInBuffLength);
    if (uiWriteLength != uiInBuffLength)
    {
        return(false);
    }
    return(true);
}

bool CTlv::Encode(CBuffer& oOutBuff)
{
    unsigned int uiWriteLength = 0;
    uiWriteLength = oOutBuff.Write(&m_uiType, sizeof(m_uiType));
    if (uiWriteLength != sizeof(m_uiType))
    {
        return(false);
    }

    uiWriteLength = oOutBuff.Write(&m_uiLength, sizeof(m_uiLength));
    if (uiWriteLength != sizeof(m_uiLength))
    {
        return(false);
    }

    uiWriteLength = oOutBuff.Write(m_oBuff.GetRawReadBuffer(), m_uiLength);
    if (uiWriteLength != m_uiLength)
    {
        return(false);
    }
    return(true);
}

bool CTlv::Decode(CBuffer& oInBuff)
{
    if (oInBuff.ReadableBytes() < sizeof(m_uiType))
    {
        return false;
    }
    size_t uiReadIdx = oInBuff.GetReadIndex();   // 读之前的位置，若读取失败，需要将读取位置重设回读之前的位置
    int iReadLength = 0;
    iReadLength = oInBuff.Read(&m_uiType, sizeof(m_uiType));
    if (iReadLength != sizeof(m_uiType))
    {
        oInBuff.SetReadIndex(uiReadIdx);
        return(false);
    }

    iReadLength = oInBuff.Read(&m_uiLength, sizeof(m_uiLength));
    if (iReadLength != sizeof(m_uiLength))
    {
        oInBuff.SetReadIndex(uiReadIdx);
        return(false);
    }

    if (oInBuff.ReadableBytes() < m_uiLength)
    {
        oInBuff.SetReadIndex(uiReadIdx);
        return false;
    }
    iReadLength = m_oBuff.Write(&oInBuff, m_uiLength);
    if (iReadLength != (int)m_uiLength)
    {
        oInBuff.SetReadIndex(uiReadIdx);
        return(false);
    }
    return(true);
}

}   // end of namespace neb

