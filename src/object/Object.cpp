/*******************************************************************************
 * Project:  Nebula
 * @file     Object.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月9日
 * @note
 * Modify history:
 ******************************************************************************/
#include "Object.hpp"
#include "session/Session.hpp"
#include "step/Step.hpp"

namespace neb
{

Object::Object(OBJECT_TYPE eObjectType)
    : m_eObjectType(eObjectType),
      m_bRegistered(false), m_ulSequence(0), m_ulTraceId(0), m_dActiveTime(0.0), m_dTimeout(0.0),
      m_pLabor(NULL), m_pLogger(NULL), m_pTimerWatcher(NULL)
{
}

Object::~Object()
{
}

/*
template <typename T>
T* Object::NewObject(T)
{
    Object* p = (Object*)new T;
    p->SetLabor(m_pLabor);
    p->SetLogger(m_pLogger);
    p->SetRegistered();
    return ((T*)p);
}
*/

bool Object::Pretreat(Object* pObject)
{
    return(m_pLabor->Pretreat(pObject));
}

bool Object::Register(Object* pObject, ev_tstamp dTimeout)
{
    switch (pObject->GetObjectType())
    {
        case OBJ_PB_STEP:
            if (OBJ_PB_STEP == GetObjectType() || OBJ_HTTP_STEP == GetObjectType() || OBJ_REDIS_STEP ==GetObjectType())
            {
                pObject->SetTraceId(GetTraceId());
                return(m_pLabor->Register(GetSequence(), (Step*)pObject, dTimeout));
            }
            else
            {
                pObject->SetTraceId(m_pLabor->GetSequence());
                return(m_pLabor->Register((Step*)pObject, dTimeout));
            }
            break;
        case OBJ_HTTP_STEP:
            break;
        case OBJ_REDIS_STEP:
            break;
        case OBJ_SESSION:
            return(m_pLabor->Register((Session*)pObject));
            break;
        case OBJ_TIMER:
            break;
        default:
            return(m_pLabor->Pretreat(pObject));
    }
    return(false);
}

void Object::Remove(Object* pObject)
{
    switch (pObject->GetObjectType())
    {
        case OBJ_PB_STEP:
            if (OBJ_PB_STEP == GetObjectType() || OBJ_HTTP_STEP == GetObjectType() || OBJ_REDIS_STEP ==GetObjectType())
            {
                return(m_pLabor->Remove(GetSequence(), (Step*)pObject));
            }
            else
            {
                return(m_pLabor->Remove((Step*)pObject));
            }
            break;
        case OBJ_HTTP_STEP:
            break;
        case OBJ_REDIS_STEP:
            break;
        case OBJ_SESSION:
            return(m_pLabor->Remove((Session*)pObject));
            break;
        case OBJ_TIMER:
            break;
        default:
            LOG4_WARN("the object can not be removed.");
            break;
    }
}

uint32 Object::GetSequence()
{
    if (!m_bRegistered)
    {
        return(0);
    }
    if (0 == m_ulSequence)
    {
        if (NULL != m_pLabor)
        {
            m_ulSequence = m_pLabor->GetSequence();
        }
    }
    return(m_ulSequence);
}

uint32 Object::GetNodeId() const
{
    return(m_pLabor->GetNodeId());
}

uint32 Object::GetWorkerIndex() const
{
    return(m_pLabor->GetWorkerIndex());
}

const std::string& Object::GetNodeType() const
{
    return(m_pLabor->GetNodeType());
}

const std::string& Object::GetWorkPath() const
{
    return(m_pLabor->GetWorkPath());
}

const std::string& Object::GetWorkerIdentify()
{
    if (m_strWorkerIdentify.size() < 5) // IP + port + worker_index长度一定会大于这个数即可，不在乎数值是什么
    {
        char szWorkerIdentify[64] = {0};
        snprintf(szWorkerIdentify, 64, "%s:%d.%d", m_pLabor->GetHostForServer().c_str(),
                        m_pLabor->GetPortForServer(), m_pLabor->GetWorkerIndex());
        m_strWorkerIdentify = szWorkerIdentify;
    }
    return(m_strWorkerIdentify);
}

const CJsonObject& Object::GetCustomConf() const
{
    return(m_pLabor->GetCustomConf());
}

time_t Object::GetNowTime() const
{
    return(m_pLabor->GetNowTime());
}

Session* Object::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(uiSessionId, strSessionClass));
}

Session* Object::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    return(m_pLabor->GetSession(strSessionId, strSessionClass));
}

bool Object::AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx)
{
    return(m_pLabor->AddNamedChannel(strIdentify, stCtx));
}

void Object::DelNamedChannel(const std::string& strIdentify)
{
    m_pLabor->DelNamedChannel(strIdentify);
}

void Object::AddInnerChannel(const tagChannelContext& stCtx)
{
    m_pLabor->AddInnerChannel(stCtx);
}

void Object::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->AddNodeIdentify(strNodeType, strIdentify);
}

void Object::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    m_pLabor->DelNodeIdentify(strNodeType, strIdentify);
}

bool Object::SendTo(const tagChannelContext& stCtx)
{
    return(m_pLabor->SendTo(stCtx));
}

bool Object::SendTo(const tagChannelContext& stCtx, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(stCtx, oMsgHead, oMsgBody));
}

bool Object::SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg)
{
    return(m_pLabor->SendTo(stCtx, oHttpMsg));
}

bool Object::SendTo(const std::string& strIdentify, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendTo(strIdentify, oMsgHead, oMsgBody));
}

bool Object::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg)
{
    return(m_pLabor->SendTo(strHost, iPort, strUrlPath, oHttpMsg, this));
}

bool Object::SendToNext(const std::string& strNodeType, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendToNext(strNodeType, oMsgHead, oMsgBody));
}

bool Object::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    return(m_pLabor->SendToWithMod(strNodeType, uiModFactor, oMsgHead, oMsgBody));
}

void Object::SetNodeId(uint32 uiNodeId)
{
    m_pLabor->SetNodeId(uiNodeId);
}

void Object::DelayTimeout()
{
    if (IsRegistered())
    {
        m_pLabor->ResetTimeout(this);
    }
    else
    {
        m_dActiveTime += m_dTimeout + 0.5;
    }
}

bool Object::SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType)
{
    return(m_pLabor->SwitchCodec(stCtx, eCodecType));
}

ev_timer* Object::AddTimerWatcher()
{
    m_pTimerWatcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (NULL != m_pTimerWatcher)
    {
        m_pTimerWatcher->data = this;    // (void*)(Channel*)
    }
    return(m_pTimerWatcher);
}

} /* namespace neb */
