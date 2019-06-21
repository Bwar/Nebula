/*******************************************************************************
 * Project:  Nebula
 * @file     PbContext.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年6月21日
 * @note
 * Modify history:
 ******************************************************************************/

#include "PbContext.hpp"

namespace neb
{

PbContext::PbContext()
    : m_iReqCmd(0), m_uiReqSeq(0)
{
}

PbContext::PbContext(
        std::shared_ptr<SocketChannel> pChannel,
        int32 iCmd, uint32 uiSeq)
    : Context(pChannel),
      m_iReqCmd(iCmd), m_uiReqSeq(uiSeq)
{
}

PbContext::PbContext(
        std::shared_ptr<SocketChannel> pChannel,
        int32 iCmd, uint32 uiSeq,
        const MsgBody& oReqMsgBody)
    : Context(pChannel),
      m_iReqCmd(iCmd), m_uiReqSeq(uiSeq),
      m_oReqMsgBody(oReqMsgBody)
{
}

PbContext::~PbContext()
{
}

bool PbContext::Response(int iErrno, const std::string& strData)
{
    MsgBody oMsgBody;
    oMsgBody.mutable_rsp_result()->set_code(iErrno);
    if (ERR_OK == iErrno)
    {
        oMsgBody.mutable_rsp_result()->set_msg("success");
        oMsgBody.set_data(strData);
    }
    else
    {
        oMsgBody.mutable_rsp_result()->set_msg(strData);
    }

    if (SendTo(GetChannel(), m_iReqCmd + 1, m_uiReqSeq, oMsgBody))
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
