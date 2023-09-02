/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelMigrate.hpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-07
 * @note     
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_MIGRATE_SOCKETCHANNELMIGRATE_HPP_
#define SRC_CHANNEL_MIGRATE_SOCKETCHANNELMIGRATE_HPP_

#include "channel/SocketChannel.hpp"
#include "SocketChannelPack.hpp"
#include "codec/Codec.hpp"
#include "channel/SpecChannel.hpp"
#include "labor/LaborShared.hpp"
#include "ios/ChannelWatcher.hpp"

namespace neb
{

class SocketChannelMigrate
{
public:
    SocketChannelMigrate();
    virtual ~SocketChannelMigrate();

    static E_CODEC_TYPE Type()
    {
        return(CODEC_CHANNEL_MIGRATE);
    }

    // request
    template<typename ...Targs>
    static int Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pSocketChannel);

    // response
    template<typename ...Targs>
    static int Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pSocketChannel);

protected:
    static bool MigrateChannel(std::shared_ptr<SocketChannel> pSocketChannel, SocketChannelPack& oPack);
};

// request
template<typename ...Targs>
int SocketChannelMigrate::Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pSocketChannel)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    SocketChannelPack oPack;
    if (!MigrateChannel(pSocketChannel, oPack))
    {
        return(ERR_SPEC_CHANNEL_MIGRATE);
    }
    std::shared_ptr<SpecChannel<SocketChannelPack>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(Type(), uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<SocketChannelPack>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            pSocketChannel.reset(oPack.UnpackChannel().get()); // recover from migration failed
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, Type());
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(oPack));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(Type(), uiFromLabor, uiToLabor, pChannel));
        }
        else
        {
            pSocketChannel.reset(oPack.UnpackChannel().get()); // recover from migration failed
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<SocketChannelPack>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            pSocketChannel.reset(oPack.UnpackChannel().get()); // recover from migration failed
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::move(oPack));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        else
        {
            pSocketChannel.reset(oPack.UnpackChannel().get()); // recover from migration failed
        }
        return(iResult);
    }
}

// response
template<typename ...Targs>
int SocketChannelMigrate::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, std::shared_ptr<SocketChannel> pSocketChannel)
{
    return(ERR_SPEC_CHANNEL_CALL);
}

} /* namespace neb */

#endif /* SRC_CHANNEL_MIGRATE_SOCKETCHANNELMIGRATE_HPP_ */

