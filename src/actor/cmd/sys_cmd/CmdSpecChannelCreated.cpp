/*******************************************************************************
 * Project:  Nebula
 * @file     CmdSpecChannelCreated.cpp
 * @brief    
 * @author   Bwar
 * @date:    2022-12-10
 * @note
 * Modify history:
 ******************************************************************************/
#include "CmdSpecChannelCreated.hpp"
#include "codec/CodecFactory.hpp"
#include "labor/LaborShared.hpp"
#include "channel/SpecChannel.hpp"

namespace neb
{

CmdSpecChannelCreated::CmdSpecChannelCreated(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdSpecChannelCreated::~CmdSpecChannelCreated()
{
}

bool CmdSpecChannelCreated::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    SpecChannelInfo oSpecInfo;
    if (!oSpecInfo.ParseFromString(oInMsgBody.data()))
    {
        LOG4_ERROR("SpecChannelInfo ParseFromString failed.");
        return(false);
    }
    LOG4_INFO("this labor %u, spec channel from %u to %u created, with codec type %u",
            GetLaborId(), oSpecInfo.from_labor(), oSpecInfo.to_labor(), oSpecInfo.codec_type());
    if (oSpecInfo.to_labor() == GetLaborId())
    {
        CodecFactory::OnSpecChannelCreated(oSpecInfo.codec_type(), oSpecInfo.from_labor(), oSpecInfo.to_labor());
    }
    else    // forward notifications through manager
    {
        LOG4_INFO("this labor %u, spec channel from %u to %u created, with codec type %u",
                GetLaborId(), oSpecInfo.from_labor(), oSpecInfo.to_labor(), oSpecInfo.codec_type());
        auto pTransitChannel = LaborShared::Instance()->GetSpecChannel(CodecNebulaInNode::Type(), GetLaborId(), oSpecInfo.to_labor());
        if (pTransitChannel == nullptr)
        {
            LOG4_ERROR("no codec_type %u channel from %u to %u", CodecNebulaInNode::Type(), GetLaborId(), oSpecInfo.to_labor());
            return(false);
        }
        else
        {
            auto pNoticeSpecChannel = std::static_pointer_cast<SpecChannel<MsgBody, MsgHead>>(pTransitChannel);
            if (pNoticeSpecChannel == nullptr)
            {
                LOG4_ERROR("spec channel cast failed.");
                return(false);
            }
            int iCmd = oInMsgHead.cmd();
            int iErrno = pNoticeSpecChannel->Write(iCmd, 0,
                    std::move(const_cast<MsgHead&>(oInMsgHead)),
                    std::move(const_cast<MsgBody&>(oInMsgBody)));
            if (iErrno == ERR_OK)
            {
                LaborShared::Instance()->GetDispatcher(oSpecInfo.to_labor())->AsyncSend(
                        pNoticeSpecChannel->MutableWatcher()->MutableAsyncWatcher());
            }
            else
            {
                LOG4_ERROR("no codec_type %u channel write error %d", CodecNebulaInNode::Type(), iErrno);
                return(false);
            }
        }
    }
    return(true);
}

} /* namespace neb */

