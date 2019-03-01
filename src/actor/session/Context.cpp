/*******************************************************************************
 * Project:  Nebula
 * @file     Context.cpp
 * @brief 
 * @author   Bwar
 * @date:    2019年2月16日
 * @note
 * Modify history:
 ******************************************************************************/

#include "Context.hpp"

namespace neb
{

Context::Context(const std::string& strSessionId, ev_tstamp dSessionTimeout)
    : Session(strSessionId, dSessionTimeout),
      m_pChannel(nullptr), m_iReqCmd(0), m_uiReqSeq(0)
{
}

Context::Context(const std::string& strSessionId,
        std::shared_ptr<SocketChannel> pChannel,
        int32 iCmd, uint32 uiSeq,
        ev_tstamp dSessionTimeout)
    : Session(strSessionId, dSessionTimeout),
      m_pChannel(pChannel), m_iReqCmd(iCmd), m_uiReqSeq(uiSeq)
{
}

Context::Context(const std::string& strSessionId,
        std::shared_ptr<SocketChannel> pChannel,
        int32 iCmd, uint32 uiSeq,
        const MsgBody& oReqMsgBody,
        ev_tstamp dSessionTimeout)
    : Session(strSessionId, dSessionTimeout),
      m_pChannel(pChannel), m_iReqCmd(iCmd), m_uiReqSeq(uiSeq),
      m_oReqMsgBody(oReqMsgBody)
{
}

Context::~Context()
{
}

bool Context::Response(int iErrno, const std::string& strData)
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

    if (SendTo(m_pChannel, m_iReqCmd + 1, m_uiReqSeq, oMsgBody))
    {
        return(true);
    }
    return(false);
}

} /* namespace neb */
