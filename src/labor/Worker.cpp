/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief 
 * @author   bwar
 * @date:    Feb 27, 2018
 * @note
 * Modify history:
 ******************************************************************************/
#include "labor/Worker.hpp"
#include "labor/WorkerImpl.hpp"

namespace neb
{

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

Worker::Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf)
    : m_pImpl(nullptr)
{
    // C++14: m_Impl = std::make_unique<WorkerImpl>(strWorkPath, iControlFd, iDataFd, iWorkerIndex, oJsonConf);
    m_pImpl = std::unique_ptr<WorkerImpl>(new WorkerImpl(this, strWorkPath, iControlFd, iDataFd, iWorkerIndex, oJsonConf));
}

Worker::~Worker()
{
    // TODO Auto-generated destructor stub
}

uint32 Worker::GetSequence() const
{
    return(m_pImpl->GetSequence());
}

Session* Worker::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pImpl->GetSession(uiSessionId, strSessionClass));
}

Session* Worker::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pImpl->GetSession(strSessionId, strSessionClass));
}

log4cplus::Logger Worker::GetLogger()
{
    return(m_pImpl->GetLogger());
}

uint32 Worker::GetNodeId() const
{
    return(m_pImpl->GetWorkerInfo().uiNodeId);
}

int Worker::GetWorkerIndex() const
{
    return(m_pImpl->GetWorkerInfo().iWorkerIndex);
}

ev_tstamp Worker::GetDefaultTimeout() const
{
    return((m_pImpl->GetWorkerInfo().dStepTimeout));
}

int Worker::GetClientBeatTime() const
{
    return((int)(m_pImpl->GetWorkerInfo().dIoTimeout));
}

const std::string& Worker::GetNodeType() const
{
    return(m_pImpl->GetWorkerInfo().strNodeType);
}

int Worker::GetPortForServer() const
{
    return(m_pImpl->GetWorkerInfo().iPortForServer);
}

const std::string& Worker::GetHostForServer() const
{
    return(m_pImpl->GetWorkerInfo().strHostForServer);
}

const std::string& Worker::GetWorkPath() const
{
    return(m_pImpl->GetWorkerInfo().strWorkPath);
}

const std::string& Worker::GetNodeIdentify() const
{
    return(m_pImpl->GetWorkerInfo().strWorkerIdentify);
}

const CJsonObject& Worker::GetCustomConf() const
{
    return(m_pImpl->GetCustomConf());
}

time_t Worker::GetNowTime() const
{
    return(m_pImpl->GetNowTime());
}

bool Worker::SendTo(const tagChannelContext& stCtx)
{
    return(m_pImpl->SendTo(stCtx));
}

bool Worker::SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendTo(stCtx, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendTo(strIdentify, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendPolling(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendPolling(strNodeType, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendOriented(const std::string& strNodeType, unsigned int uiFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendOriented(strNodeType, uiFactor, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendOriented(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendOriented(strNodeType, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->Broadcast(strNodeType, uiCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    return(m_pImpl->SendTo(stCtx, oHttpMsg, uiHttpStepSeq));
}

bool Worker::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    return(m_pImpl->SendTo(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
}

bool Worker::SendTo(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    return(m_pImpl->SendTo(strHost, iPort, pRedisStep));
}

std::string Worker::GetClientAddr(const tagChannelContext& stCtx)
{
    return(m_pImpl->GetClientAddr(stCtx));
}

} /* namespace neb */
