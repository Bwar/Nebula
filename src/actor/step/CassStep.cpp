/*******************************************************************************
 * Project:  Nebula
 * @file     CassStep.cpp
 * @brief 
 * @author   Bwar
 * @date:    2021-12-11
 * @note
 * Modify history:
 ******************************************************************************/
#include "actor/step/CassStep.hpp"

namespace neb
{

CassStep::CassStep(ev_tstamp dTimeout)
    : Step(ACT_REDIS_STEP, dTimeout)
{
}

CassStep::~CassStep()
{
}

E_CMD_STATUS CassStep::Callback(std::shared_ptr<SocketChannel> pChannel,
        const CassMessage& oCassMsg)
{
    auto& oCassResponse = static_cast<const CassResponse&>(oCassMsg);
    if (oCassResponse.IsSuccess())
    {
        return(Callback(pChannel, oCassResponse.GetResult(), oCassResponse.GetErrCode(), oCassResponse.GetErrMsg()));
    }
    else
    {
        if (oCassResponse.GetErrCode() == 0)
        {
            return(Callback(pChannel, oCassResponse.GetResult(), -1, oCassResponse.GetErrMsg()));
        }
        else
        {
            return(Callback(pChannel, oCassResponse.GetResult(), oCassResponse.GetErrCode(), oCassResponse.GetErrMsg()));
        }
    }
}

} /* namespace neb */
