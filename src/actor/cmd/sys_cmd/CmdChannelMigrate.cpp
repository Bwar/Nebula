/*******************************************************************************
 * Project:  Nebula
 * @file     CmdChannelMigrate.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-08
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CmdChannelMigrate.hpp"
#include "channel/SocketChannel.hpp"
#include "ios/Dispatcher.hpp"
#include "ios/ChannelWatcher.hpp"
#include "actor/step/Step.hpp"

namespace neb
{

CmdChannelMigrate::CmdChannelMigrate(int32 iCmd)
    : Cmd(iCmd)
{
}

CmdChannelMigrate::~CmdChannelMigrate()
{
}

bool CmdChannelMigrate::AnyMessage(
            std::shared_ptr<SocketChannel> pChannel, SocketChannelPack& oPack)
{
    auto pMigratedChannel = oPack.UnpackChannel();
    GetLabor(this)->GetDispatcher()->AddChannelToLoop(pMigratedChannel);
    LOG4_INFO("channel[%d] with codec_type %d migrate done.",
            pMigratedChannel->GetFd(), pMigratedChannel->GetCodecType());
    switch (pMigratedChannel->GetCodecType())
    {
        case CODEC_NEBULA:
        {
            auto pStepTellWorker = MakeSharedStep("neb::StepTellWorker", pMigratedChannel);
            if (nullptr == pStepTellWorker)
            {
                return(false);
            }
            pStepTellWorker->Emit();
        }
            break;
        case CODEC_NEBULA_IN_NODE:
            pMigratedChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            break;
        default:
            pMigratedChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    }
    GetLabor(this)->GetDispatcher()->MigrateChannelRecvAndHandle(pMigratedChannel);
    return(true);
}

} /* namespace neb */

