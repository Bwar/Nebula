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
    m_pImpl = new WorkerImpl(this, strWorkPath, iControlFd, iDataFd, iWorkerIndex, oJsonConf);
}

Worker::~Worker()
{
    DELETE(m_pImpl);
}

void Worker::Run()
{
    m_pImpl->Run();
}

uint32 Worker::GetSequence() const
{
    return(m_pImpl->GetSequence());
}

std::shared_ptr<Session> Worker::GetSession(uint32 uiSessionId)
{
    return(m_pImpl->GetSession(uiSessionId));
}

std::shared_ptr<Session> Worker::GetSession(const std::string& strSessionId)
{
    return(m_pImpl->GetSession(strSessionId));
}

bool Worker::ExecStep(uint32 uiStepSeq, int iErrno, const std::string& strErrMsg, void* data)
{
    return(m_pImpl->ExecStep(uiStepSeq, iErrno, strErrMsg, data));
}

void Worker::AddAssemblyLine(std::shared_ptr<Session> pSession)
{
    m_pImpl->AddAssemblyLine(pSession);
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

bool Worker::AddNetLogMsg(const MsgBody& oMsgBody) 
{
    return(m_pImpl->AddNetLogMsg(oMsgBody));
}

bool Worker::SendTo(std::shared_ptr<SocketChannel> pChannel)
{
    return(m_pImpl->SendTo(pChannel));
}

bool Worker::SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendTo(pChannel, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendTo(strIdentify, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendRoundRobin(strNodeType, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendOriented(const std::string& strNodeType, unsigned int uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendOriented(strNodeType, uiFactor, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->SendOriented(strNodeType, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::Broadcast(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody, Actor* pSender)
{
    return(m_pImpl->Broadcast(strNodeType, iCmd, uiSeq, oMsgBody, pSender));
}

bool Worker::SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    return(m_pImpl->SendTo(pChannel, oHttpMsg, uiHttpStepSeq));
}

bool Worker::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    return(m_pImpl->SendTo(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
}

bool Worker::SendTo(std::shared_ptr<RedisChannel> pChannel, Actor* pSender)
{
    return(m_pImpl->SendTo(pChannel, pSender));
}

bool Worker::SendTo(const std::string& strIdentify, Actor* pSender)
{
    return(m_pImpl->SendTo(strIdentify, pSender));
}

bool Worker::SendTo(const std::string& strHost, int iPort, Actor* pSender)
{
    return(m_pImpl->SendTo(strHost, iPort, pSender));
}

} /* namespace neb */
