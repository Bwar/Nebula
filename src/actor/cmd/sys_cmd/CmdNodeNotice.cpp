/*******************************************************************************
 * Project:  Nebula
 * @file     CmdNodeNotice.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月16日
 * @note
 * Modify history:
 ******************************************************************************/

#include <actor/cmd/sys_cmd/CmdNodeNotice.hpp>
#include "util/json/CJsonObject.hpp"

namespace neb
{

CmdNodeNotice::CmdNodeNotice(int32 iCmd)
    : Cmd(iCmd), pStepNodeNotice(NULL)
{
}

CmdNodeNotice::~CmdNodeNotice()
{
}

bool CmdNodeNotice::AnyMessage(
                const tagChannelContext& stCtx,
                const MsgHead& oInMsgHead,
                const MsgBody& oInMsgBody)
{
    CBuffer oBuff;
    MsgHead oOutMsgHead;
    MsgBody oOutMsgBody;

    CJsonObject oJson;
    if (oJson.Parse(oInMsgBody.data()))
    {
        LOG4_DEBUG("CmdNodeNotice seq[%llu] jsonbuf[%s] Parse is ok",
            oInMsgHead.seq(),oInMsgBody.data().c_str());

        std::shared_ptr<Step> pStep = MakeSharedStep("neb::StepNodeNotice", oInMsgBody);
        if (nullptr == pStep)
        {
            LOG4_ERROR("error %d: new StepNodeNotice() error!", ERR_NEW);
            return(false);
        }
        pStep->Emit(ERR_OK);
        return(true);
    }
    else
    {
        LOG4_ERROR("failed to parse %s", oInMsgBody.data().c_str());
    }

    return(false);
}

} /* namespace neb */
