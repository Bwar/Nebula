/*******************************************************************************
 * Project:  Nebula
 * @file     CodecDeliver.cpp
 * @brief    
 * @author   Bwar
 * @date:    2023-01-25
 * @note     
 * Modify history:
 ******************************************************************************/
#include "CodecDeliver.hpp"

namespace neb
{

CodecDeliver::CodecDeliver()
{
}

CodecDeliver::~CodecDeliver()
{
}

// request
int CodecDeliver::Write(uint32 uiFromLabor, uint32 uiToLabor, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage)
{
    if (uiFromLabor == uiToLabor)
    {
        return(ERR_SPEC_CHANNEL_TARGET);
    }
    std::shared_ptr<SpecChannel<Package>> pSpecChannel = nullptr;
    auto pLaborShared = LaborShared::Instance();
    auto pChannel = pLaborShared->GetSpecChannel(Type(), uiFromLabor, uiToLabor);
    if (pChannel == nullptr)
    {
        pSpecChannel = std::make_shared<SpecChannel<Package>>(
                uiFromLabor, uiToLabor, pLaborShared->GetSpecChannelQueueSize(), true);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CREATE);
        }
        pChannel = std::dynamic_pointer_cast<SocketChannel>(pSpecChannel);
        auto pWatcher = pSpecChannel->MutableWatcher();
        pWatcher->Set(pChannel, Type());
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<Package>(oPackage));
        if (iResult == ERR_OK)
        {
            return(pLaborShared->AddSpecChannel(Type(), uiFromLabor, uiToLabor, pChannel));
        }
        return(iResult);
    }
    else
    {
        pSpecChannel = std::static_pointer_cast<SpecChannel<Package>>(pChannel);
        if (pSpecChannel == nullptr)
        {
            return(ERR_SPEC_CHANNEL_CAST);
        }
        int iResult = pSpecChannel->Write(uiFlags, uiStepSeq, std::forward<Package>(oPackage));
        if (iResult == ERR_OK)
        {
            pLaborShared->GetDispatcher(uiToLabor)->AsyncSend(pSpecChannel->MutableWatcher()->MutableAsyncWatcher());
        }
        return(iResult);
    }
}

// response
int CodecDeliver::Write(std::shared_ptr<SocketChannel> pChannel, uint32 uiFlags, uint32 uiStepSeq, Package&& oPackage)
{
    uint32 uiFrom;
    uint32 uiTo;
    std::static_pointer_cast<SpecChannel<Package>>(pChannel)->GetEnds(uiFrom, uiTo);
    return(Write(uiTo, uiFrom, uiFlags, uiStepSeq, std::forward<Package>(oPackage)));
}

} /* namespace neb */

