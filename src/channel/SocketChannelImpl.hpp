/*******************************************************************************
 * Project:  Nebula
 * @file     SocketChannelImpl.hpp
 * @brief    Socket通信通道实现
 * @author   Bwar
 * @date:    2016年8月10日
 * @note     Socket通信通道实现
 * Modify history:
 ******************************************************************************/
#ifndef SRC_CHANNEL_SOCKETCHANNELIMPL_HPP_
#define SRC_CHANNEL_SOCKETCHANNELIMPL_HPP_

#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include "ev.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "util/CBuffer.hpp"
#include "util/StreamCodec.hpp"
#include "util/json/CJsonObject.hpp"

#include "codec/Codec.hpp"
#include "Channel.hpp"
#include "SocketChannel.hpp"
#include "Definition.hpp"
#include "logger/NetLogger.hpp"
#include "actor/Actor.hpp"
#include "ios/ChannelWatcher.hpp"
#include "labor/NodeInfo.hpp"

namespace neb
{

class Labor;
class NetLogger;
class CodecFactory;
class Dispatcher;
class SocketChannel;
class Step;

template<typename T>
class SocketChannelImpl: public SocketChannel
{
public:
    SocketChannelImpl(Labor* pLabor, std::shared_ptr<NetLogger> pLogger, int iFd, uint32 ulSeq, ev_tstamp dKeepAlive = 0.0);
    virtual ~SocketChannelImpl();

    static bool NewCodec(std::shared_ptr<SocketChannel> pChannel, Labor* pLabor, std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType);

    template <typename ...Targs>
    E_CODEC_STATUS SendRequest(uint32 uiStepSeq, Targs&&... args);

    template <typename ...Targs>
    E_CODEC_STATUS SendResponse(Targs&&... args);

    template <typename ...Targs>
    E_CODEC_STATUS Recv(Targs&&... args);

    template <typename ...Targs>
    E_CODEC_STATUS Fetch(Targs&&... args);

    E_CODEC_STATUS Send();

    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);

public:
    virtual int GetFd() const
    {
        return(m_iFd);
    }

    virtual uint32 GetSequence() const
    {
        return(m_uiSeq);
    }

    virtual bool IsPipeline() const
    {
        return(m_bPipeline);
    }

    virtual const std::string& GetIdentify() const
    {
        return(m_strIdentify);
    }

    virtual const std::string& GetRemoteAddr() const
    {
        return(m_strRemoteAddr);
    }

    virtual const std::string& GetClientData() const
    {
        return(m_strClientData);
    }

    virtual E_CODEC_TYPE GetCodecType() const;

    virtual uint8 GetChannelStatus() const
    {
        return(m_ucChannelStatus);
    }

    virtual uint32 PopStepSeq(uint32 uiStreamId = 0, E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK);

    virtual bool PipelineIsEmpty() const
    {
        return(m_listPipelineStepSeq.empty());
    }

    virtual ev_tstamp GetActiveTime() const
    {
        return(m_dActiveTime);
    }

    virtual ev_tstamp GetKeepAlive();

    virtual int GetErrno() const
    {
        return(m_iErrno);
    }

    virtual const std::string& GetErrMsg() const
    {
        return(m_strErrMsg);
    }

    virtual Codec* GetCodec() const
    {
        return(m_pCodec);
    }

    virtual Labor* GetLabor()
    {
        return(m_pLabor);
    }

    int16 GetRemoteWorkerIndex() const
    {
        return(m_iRemoteWorkerIdx);
    }

    bool IsChannelVerify() const
    {
        return(m_strClientData.size());
    }

    bool NeedAliveCheck() const;

    uint32 GetMsgNum() const
    {
        return(m_uiMsgNum);
    }

    uint32 GetUnitTimeMsgNum() const
    {
        return(m_uiUnitTimeMsgNum);
    }

    const std::list<uint32>& GetPipelineStepSeq() const
    {
        return(m_listPipelineStepSeq);
    }

    const std::unordered_map<uint32, uint32>& GetStreamStepSeq() const
    {
        return(m_mapStreamStepSeq);
    }

public:
    /*
    void SetActiveTime(ev_tstamp dTime)
    {
        m_dActiveTime = dTime;
    }
    */

    void SetKeepAlive(ev_tstamp dTime)
    {
        m_dKeepAlive = dTime;
    }

    void SetChannelStatus(E_CHANNEL_STATUS eStatus)
    {
        m_ucChannelStatus = (uint8)eStatus;
    }

    void SetPipeline(bool bPipeline)
    {
        m_bPipeline = bPipeline;
    }

    void SetClientData(const std::string& strClientData)
    {
        m_strClientData = strClientData;
    }

    void SetIdentify(const std::string& strIdentify)
    {
        m_strIdentify = strIdentify;
    }

    void SetRemoteAddr(const std::string& strRemoteAddr)
    {
        m_strRemoteAddr = strRemoteAddr;
    }

    void SetSecretKey(const std::string& strKey);

    void SetRemoteWorkerIndex(int16 iRemoteWorkerIndex);

    virtual bool Close();

protected:
    virtual int Write(CBuffer* pBuff, int& iErrno);
    virtual int Read(CBuffer* pBuff, int& iErrno);

private:
    bool SetCodec(Codec* pCodec);

private:
    uint8 m_ucChannelStatus;
    E_CODEC_STATUS m_eLastCodecStatus;    ///< 连接关闭前的最后一个编解码状态（当且仅当连接的应用层读缓冲区有数据未处理完而对端关闭连接时使用）
    char m_szErrBuff[256];
    int16 m_iRemoteWorkerIdx;           ///< 对端Worker进程ID,若不涉及则无需关心
    int32 m_iFd;                          ///< 文件描述符
    uint32 m_uiSeq;                       ///< 文件描述符创建时对应的序列号
    uint32 m_bPipeline;                   ///< 是否支持pipeline
    uint32 m_uiUnitTimeMsgNum;            ///< 统计单位时间内接收消息数量
    uint32 m_uiMsgNum;                    ///< 接收消息数量
    ev_tstamp m_dActiveTime;              ///< 最后一次访问时间
    ev_tstamp m_dKeepAlive;               ///< 连接保持时间
    CBuffer* m_pRecvBuff;
    CBuffer* m_pSendBuff;
    CBuffer* m_pWaitForSendBuff;    ///< 等待发送的数据缓冲区（数据到达时，连接并未建立，等连接建立并且pSendBuff发送完毕后立即发送）
    Codec* m_pCodec;                      ///< 编解码器
    HttpMsg* m_pHoldingHttpMsg;           // 如果有http协议转换
    int m_iErrno;
    std::string m_strKey;                 ///< 密钥
    std::string m_strClientData;         ///< 客户端相关数据（例如IM里的用户昵称、头像等，登录或连接时保存起来，后续发消息或其他操作无须客户端再带上来）
    std::string m_strErrMsg;
    std::string m_strIdentify;            ///< 连接标识（可以为空，不为空时用于标识业务层与连接的关系）
    std::string m_strRemoteAddr;          ///< 对端IP地址（不是客户端地址，但可能跟客户端地址相同）
    std::list<uint32> m_listPipelineStepSeq;  ///< 等待回调的Step seq
    std::unordered_map<uint32, uint32> m_mapStreamStepSeq;      ///< 等待回调的http2 step seq
    std::set<E_CODEC_TYPE> m_setSkipCodecType;  ///< Codec转换需跳过的CodecType
    Labor* m_pLabor;
    std::shared_ptr<NetLogger> m_pLogger;

    friend class CodecFactory;
    friend class Dispatcher;
};

template<typename T>
SocketChannelImpl<T>::SocketChannelImpl(Labor* pLabor, std::shared_ptr<NetLogger> pLogger,
        int iFd, uint32 ulSeq, ev_tstamp dKeepAlive)
    : m_ucChannelStatus(CHANNEL_STATUS_INIT),m_eLastCodecStatus(CODEC_STATUS_OK),
      m_iRemoteWorkerIdx(-1), m_iFd(iFd), m_uiSeq(ulSeq), m_bPipeline(true),
      m_uiUnitTimeMsgNum(0), m_uiMsgNum(0),
      m_dActiveTime(0.0), m_dKeepAlive(dKeepAlive),
      m_pRecvBuff(nullptr), m_pSendBuff(nullptr), m_pWaitForSendBuff(nullptr),
      m_pCodec(nullptr), m_pHoldingHttpMsg(nullptr), m_iErrno(0), m_pLabor(pLabor),
      m_pLogger(pLogger)
{
    try
    {
        m_pRecvBuff = new CBuffer();
        m_pSendBuff = new CBuffer();
        m_pWaitForSendBuff = new CBuffer();
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("%s", e.what());
    }
    //m_dActiveTime = m_pLabor->GetNowTime();
    memset(m_szErrBuff, 0, sizeof(m_szErrBuff));
}

template<typename T>
SocketChannelImpl<T>::~SocketChannelImpl()
{
    LOG4_TRACE("SocketChannelImpl::~SocketChannelImpl() fd %d, seq %u", m_iFd, m_uiSeq);
    m_listPipelineStepSeq.clear();
    m_mapStreamStepSeq.clear();
    if (CHANNEL_STATUS_CLOSED != m_ucChannelStatus)
    {
        Close();
    }
    FREE(m_pWatcher);
    DELETE(m_pRecvBuff);
    DELETE(m_pSendBuff);
    DELETE(m_pWaitForSendBuff);
    DELETE(m_pHoldingHttpMsg);
    DELETE(m_pCodec);
}

template<typename T>
E_CODEC_TYPE SocketChannelImpl<T>::GetCodecType() const
{
    if (m_pCodec == nullptr)
    {
        return(CODEC_UNKNOW);
    }
    return(m_pCodec->GetCodecType());
}

template<typename T>
uint32 SocketChannelImpl<T>::PopStepSeq(uint32 uiStreamId, E_CODEC_STATUS eCodecStatus)
{
    if (m_listPipelineStepSeq.empty())
    {
        auto iter = m_mapStreamStepSeq.find(uiStreamId);
        if (iter != m_mapStreamStepSeq.end())
        {
            uint32 uiStepSeq = iter->second;
            if (CODEC_STATUS_OK == eCodecStatus)
            {
                m_mapStreamStepSeq.erase(iter);
            }
            return(uiStepSeq);
        }
        return(0);
    }
    else
    {
        uint32 uiStepSeq = m_listPipelineStepSeq.front();
        if (CODEC_STATUS_OK == eCodecStatus)
        {
            m_listPipelineStepSeq.pop_front();
        }
        return(uiStepSeq);
    }
}

template<typename T>
ev_tstamp SocketChannelImpl<T>::GetKeepAlive()
{
    if (m_pCodec == nullptr)
    {
        LOG4_ERROR("no codec found, please check codec type is valid.");
        return(0.0);
    }
    if (CODEC_HTTP == m_pCodec->GetCodecType())
    {
        if ((static_cast<T*>(m_pCodec))->GetKeepAlive() >= 0.0)
        {
            return((static_cast<T*>(m_pCodec))->GetKeepAlive());
        }
    }
    return(m_dKeepAlive);
}

template<typename T>
bool SocketChannelImpl<T>::NeedAliveCheck() const
{
    if (CODEC_HTTP == m_pCodec->GetCodecType()
            || CODEC_NEBULA == m_pCodec->GetCodecType()
            || CODEC_NEBULA_IN_NODE == m_pCodec->GetCodecType())
    {
        return(false);
    }
    return(true);
}

template<typename T>
template <typename ...Targs>
void SocketChannelImpl<T>::Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args)
{
    m_pLogger->WriteLog(iLogLevel, szFileName, uiFileLine, szFunction, std::forward<Targs>(args)...);
}

template<typename T>
bool SocketChannelImpl<T>::NewCodec(std::shared_ptr<SocketChannel> pChannel, Labor* pLabor, std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType)
{
    if (T::Type() != eCodecType)
    {
        pLogger->WriteLog(neb::Logger::INFO, __FILE__, __LINE__, __FUNCTION__,
                "Codec %d with codec type %d not match needed codec type %d",
                T::Type(), eCodecType);
        return(false);
    }
    Codec* pCodec = nullptr;
    try
    {
        pCodec = new T(pLogger, eCodecType, pChannel);
    }
    catch(std::bad_alloc& e)
    {
        return(false);
    }
    if (pChannel->m_pImpl == nullptr)
    {
        auto pImpl = std::make_shared<SocketChannelImpl<T>>(pLabor, pLogger, pChannel->GetFd(), eCodecType);
        pImpl->SetCodec(pCodec);
        pChannel->InitImpl(std::static_pointer_cast<SocketChannel>(pImpl));
    }
    else
    {
        std::static_pointer_cast<SocketChannelImpl<T>>(pChannel->m_pImpl)->SetCodec(pCodec);
    }
    return(true);

}

template<typename T>
E_CODEC_STATUS SocketChannelImpl<T>::Send()
{
    LOG4_TRACE("channel_fd[%d], channel_seq[%d], channel_status[%d]", m_iFd, m_uiSeq, (int)m_ucChannelStatus);
    if (CHANNEL_STATUS_CLOSED == m_ucChannelStatus || CHANNEL_STATUS_BROKEN == m_ucChannelStatus)
    {
        LOG4_WARNING("%s channel_fd[%d], channel_seq[%d], channel_status[%d] remote %s EOF.",
                m_strIdentify.c_str(), m_iFd, m_uiSeq, (int)m_ucChannelStatus, m_strRemoteAddr.c_str());
        return(CODEC_STATUS_ERR);
    }
    int iNeedWriteLen = 0;
    iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (0 == iNeedWriteLen)
    {
        if (CHANNEL_STATUS_ESTABLISHED != m_ucChannelStatus)
        {
            return(CODEC_STATUS_OK);
        }
        iNeedWriteLen = m_pWaitForSendBuff->ReadableBytes();
        if (0 == iNeedWriteLen)
        {
            LOG4_TRACE("no data need to send.");
            return(CODEC_STATUS_OK);
        }
        else
        {
            CBuffer* pExchangeBuff = m_pSendBuff;
            m_pSendBuff = m_pWaitForSendBuff;
            m_pWaitForSendBuff = pExchangeBuff;
            m_pWaitForSendBuff->Compact(1);
        }
    }

    if (m_uiMsgNum < 1)
    {
        m_dKeepAlive = m_pLabor->GetNodeInfo().dIoTimeout;
    }
    m_dActiveTime = m_pLabor->GetNowTime();
    int iHadWrittenLen = 0;
    int iWrittenLen = 0;
    do
    {
        iWrittenLen = Write(m_pSendBuff, m_iErrno);
        if (iWrittenLen > 0)
        {
            iHadWrittenLen += iWrittenLen;
        }
    }
    while (iWrittenLen > 0 && iHadWrittenLen < iNeedWriteLen);
    LOG4_TRACE("iNeedWriteLen = %d, iHadWrittenLen = %d", iNeedWriteLen, iHadWrittenLen);
    if (iHadWrittenLen >= 0)
    {
        m_pLabor->IoStatAddSendBytes(m_iFd, iHadWrittenLen);
        if (m_pSendBuff->Capacity() > CBuffer::BUFFER_MAX_READ
            && (m_pSendBuff->ReadableBytes() < m_pSendBuff->Capacity() / 2))
        {
            m_pSendBuff->Compact(m_pSendBuff->ReadableBytes() * 2);
        }
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iHadWrittenLen && 0 == m_pWaitForSendBuff->ReadableBytes())
        {
            return(CODEC_STATUS_OK);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno || EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        m_strErrMsg = strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff));
        LOG4_ERROR("send to %s[fd %d] error %d: %s", m_strIdentify.c_str(),
                m_iFd, m_iErrno, m_strErrMsg.c_str());
        m_ucChannelStatus = CHANNEL_STATUS_BROKEN;
        return(CODEC_STATUS_INT);
    }
}

template<typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelImpl<T>::SendRequest(uint32 uiStepSeq, Targs&&... args)
{
    LOG4_TRACE("channel_fd[%d], channel_seq[%d], channel_status[%d]", m_iFd, m_uiSeq, (int)m_ucChannelStatus);
    if (m_pCodec == nullptr)
    {
        LOG4_ERROR("no codec found, please check whether the CODEC_TYPE is valid.");
        return(CODEC_STATUS_ERR);
    }
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    switch (m_ucChannelStatus)
    {
        case CHANNEL_STATUS_ESTABLISHED:
            eCodecStatus = (static_cast<T*>(m_pCodec))->Encode(std::forward<Targs>(args)..., m_pSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus && uiStepSeq > 0)
            {
                uint32 uiStreamId = (static_cast<T*>(m_pCodec))->GetLastStreamId();
                if (0 == uiStreamId)
                {
                    m_listPipelineStepSeq.push_back(uiStepSeq);
                }
                else
                {
                    m_mapStreamStepSeq.insert(std::make_pair(uiStreamId, uiStepSeq));
                }
            }
            break;
        case CHANNEL_STATUS_CLOSED:
            LOG4_WARNING("%s channel_fd[%d], channel_seq[%d], channel_status[%d] remote %s EOF.",
                    m_strIdentify.c_str(), m_iFd, m_uiSeq, (int)m_ucChannelStatus, m_strRemoteAddr.c_str());
            return(CODEC_STATUS_ERR);
        case CHANNEL_STATUS_TELL_WORKER:
        case CHANNEL_STATUS_WORKER:
        case CHANNEL_STATUS_TRANSFER_TO_WORKER:
        case CHANNEL_STATUS_CONNECTED:
        case CHANNEL_STATUS_TRY_CONNECT:
        case CHANNEL_STATUS_INIT:
            eCodecStatus = (static_cast<T*>(m_pCodec))->Encode(std::forward<Targs>(args)..., m_pWaitForSendBuff);
            if (CODEC_STATUS_OK == eCodecStatus && uiStepSeq > 0)
            {
                eCodecStatus = CODEC_STATUS_PAUSE;
                uint32 uiStreamId = (static_cast<T*>(m_pCodec))->GetLastStreamId();
                if (0 == uiStreamId)
                {
                    m_listPipelineStepSeq.push_back(uiStepSeq);
                }
                else
                {
                    m_mapStreamStepSeq.insert(std::make_pair(uiStreamId, uiStepSeq));
                }
            }
            break;
        default:
            LOG4_ERROR("%s invalid connection status %d!", m_strIdentify.c_str(), (int)m_ucChannelStatus);
            return(CODEC_STATUS_ERR);
    }

    if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PART_OK != eCodecStatus)
    {
        return(eCodecStatus);
    }

    int iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (iNeedWriteLen <= 0)
    {
        return(eCodecStatus);
    }

    int iHadWrittenLen = 0;
    int iWrittenLen = 0;
    do
    {
        iWrittenLen = Write(m_pSendBuff, m_iErrno);
        if (iWrittenLen > 0)
        {
            iHadWrittenLen += iWrittenLen;
        }
    }
    while (iWrittenLen > 0 && iHadWrittenLen < iNeedWriteLen);
    LOG4_TRACE("iNeedWriteLen = %d, iHadWrittenLen = %d", iNeedWriteLen, iHadWrittenLen);
    if (iHadWrittenLen >= 0)
    {
        m_pLabor->IoStatAddSendBytes(m_iFd, iHadWrittenLen);
        if (m_pSendBuff->Capacity() > CBuffer::BUFFER_MAX_READ
            && (m_pSendBuff->ReadableBytes() < m_pSendBuff->Capacity() / 2))
        {
            m_pSendBuff->Compact(m_pSendBuff->ReadableBytes() * 2);
        }
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iHadWrittenLen)
        {
            return(eCodecStatus);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno || EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        m_strErrMsg = strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff));
        LOG4_ERROR("send to %s[fd %d] error %d: %s", m_strIdentify.c_str(),
                m_iFd, m_iErrno, m_strErrMsg.c_str());
        m_ucChannelStatus = CHANNEL_STATUS_BROKEN;
        return(CODEC_STATUS_INT);
    }
}

template<typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelImpl<T>::SendResponse(Targs&&... args)
{
    LOG4_TRACE("channel_fd[%d], channel_seq[%d], channel_status[%d]", m_iFd, m_uiSeq, (int)m_ucChannelStatus);
    if (m_pCodec == nullptr)
    {
        LOG4_ERROR("no codec found, please check whether the CODEC_TYPE is valid.");
        return(CODEC_STATUS_ERR);
    }
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    switch (m_ucChannelStatus)
    {
        case CHANNEL_STATUS_ESTABLISHED:
            eCodecStatus = (static_cast<T*>(m_pCodec))->Encode(std::forward<Targs>(args)..., m_pSendBuff);
            break;
        case CHANNEL_STATUS_CLOSED:
            LOG4_WARNING("%s channel_fd[%d], channel_seq[%d], channel_status[%d] remote %s EOF.",
                    m_strIdentify.c_str(), m_iFd, m_uiSeq, (int)m_ucChannelStatus, m_strRemoteAddr.c_str());
            return(CODEC_STATUS_ERR);
        case CHANNEL_STATUS_TELL_WORKER:
        case CHANNEL_STATUS_WORKER:
        case CHANNEL_STATUS_TRANSFER_TO_WORKER:
        case CHANNEL_STATUS_CONNECTED:
        case CHANNEL_STATUS_TRY_CONNECT:
        case CHANNEL_STATUS_INIT:
            eCodecStatus = (static_cast<T*>(m_pCodec))->Encode(std::forward<Targs>(args)..., m_pWaitForSendBuff);
            break;
        default:
            LOG4_ERROR("%s invalid connection status %d!", m_strIdentify.c_str(), (int)m_ucChannelStatus);
            return(CODEC_STATUS_ERR);
    }

    if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PART_OK != eCodecStatus)
    {
        return(eCodecStatus);
    }

    int iNeedWriteLen = m_pSendBuff->ReadableBytes();
    if (iNeedWriteLen <= 0)
    {
        return(eCodecStatus);
    }

    int iHadWrittenLen = 0;
    int iWrittenLen = 0;
    do
    {
        iWrittenLen = Write(m_pSendBuff, m_iErrno);
        if (iWrittenLen > 0)
        {
            iHadWrittenLen += iWrittenLen;
        }
    }
    while (iWrittenLen > 0 && iHadWrittenLen < iNeedWriteLen);
    LOG4_TRACE("iNeedWriteLen = %d, iHadWrittenLen = %d", iNeedWriteLen, iHadWrittenLen);
    if (iHadWrittenLen >= 0)
    {
        m_pLabor->IoStatAddSendBytes(m_iFd, iHadWrittenLen);
        if (m_pSendBuff->Capacity() > CBuffer::BUFFER_MAX_READ
            && (m_pSendBuff->ReadableBytes() < m_pSendBuff->Capacity() / 2))
        {
            m_pSendBuff->Compact(m_pSendBuff->ReadableBytes() * 2);
        }
        m_dActiveTime = m_pLabor->GetNowTime();
        if (iNeedWriteLen == iHadWrittenLen)
        {
            if (m_pCodec->GetCodecType() == CODEC_HTTP
                    && (static_cast<T*>(m_pCodec))->GetKeepAlive() == 0.0)
            {
                return(CODEC_STATUS_EOF);
            }
            return(eCodecStatus);
        }
        else
        {
            return(CODEC_STATUS_PAUSE);
        }
    }
    else
    {
        if (EAGAIN == m_iErrno || EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            return(CODEC_STATUS_PAUSE);
        }
        m_strErrMsg = strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff));
        LOG4_ERROR("send to %s[fd %d] error %d: %s", m_strIdentify.c_str(),
                m_iFd, m_iErrno, m_strErrMsg.c_str());
        m_ucChannelStatus = CHANNEL_STATUS_BROKEN;
        return(CODEC_STATUS_INT);
    }
}

template<typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelImpl<T>::Recv(Targs&&... args)
{
    LOG4_TRACE("channel_fd[%d], channel_seq[%d]", m_iFd, m_uiSeq);
    if (CHANNEL_STATUS_CLOSED == m_ucChannelStatus || CHANNEL_STATUS_BROKEN == m_ucChannelStatus)
    {
        LOG4_WARNING("%s channel_fd[%d], channel_seq[%d], channel_status[%d] remote %s EOF.",
                m_strIdentify.c_str(), m_iFd, m_uiSeq, (int)m_ucChannelStatus, m_strRemoteAddr.c_str());
        return(CODEC_STATUS_ERR);
    }
    if (m_pCodec == nullptr)
    {
        LOG4_ERROR("no codec found, please check whether the CODEC_TYPE is valid.");
        return(CODEC_STATUS_ERR);
    }
    int iReadLen = 0;
    int iHadReadLen = 0;
    do
    {
        iReadLen = Read(m_pRecvBuff, m_iErrno);
        LOG4_TRACE("recv from fd %d data len %d. and m_pRecvBuff->ReadableBytes() = %d",
                m_iFd, iReadLen, m_pRecvBuff->ReadableBytes());
        if (iReadLen > 0)
        {
            iHadReadLen += iReadLen;
        }
    }
    while (iReadLen > 0);
    m_pLabor->IoStatAddRecvBytes(m_iFd, iHadReadLen);

    if (iReadLen == 0)
    {
        m_eLastCodecStatus = CODEC_STATUS_EOF;
    }
    else if (iReadLen < 0)
    {
        if (EAGAIN == m_iErrno || EINTR == m_iErrno)    // 对非阻塞socket而言，EAGAIN不是一种错误;EINTR即errno为4，错误描述Interrupted system call，操作也应该继续。
        {
            m_dActiveTime = m_pLabor->GetNowTime();
            m_eLastCodecStatus = CODEC_STATUS_PAUSE;
        }
        else
        {
            m_strErrMsg = strerror_r(m_iErrno, m_szErrBuff, sizeof(m_szErrBuff));
            LOG4_ERROR("recv from %s[fd %d] remote %s error %d: %s",
                    m_strIdentify.c_str(), m_iFd, m_strRemoteAddr.c_str(),
                    m_iErrno, m_strErrMsg.c_str());
            m_eLastCodecStatus = CODEC_STATUS_INT;
            m_ucChannelStatus = CHANNEL_STATUS_BROKEN;
        }
    }

    if (iHadReadLen > 0)
    {
        if (m_pRecvBuff->Capacity() > CBuffer::BUFFER_MAX_READ
            && (m_pRecvBuff->ReadableBytes() < m_pRecvBuff->Capacity() / 2))
        {
            m_pRecvBuff->Compact(m_pRecvBuff->ReadableBytes() * 2);
        }
        m_dActiveTime = m_pLabor->GetNowTime();
        auto uiReadIndex = m_pRecvBuff->GetReadIndex();
        E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
        if (m_pCodec->DecodeWithReactor())
        {
            size_t uiSendBuffLen = m_pSendBuff->ReadableBytes();
            eCodecStatus = (static_cast<T*>(m_pCodec))->Decode(m_pRecvBuff, std::forward<Targs>(args)..., m_pSendBuff);
            if (m_pSendBuff->ReadableBytes() > uiSendBuffLen
                    && (eCodecStatus == CODEC_STATUS_OK || eCodecStatus == CODEC_STATUS_PART_OK))
            {
                Send();
            }
        }
        else
        {
            eCodecStatus = (static_cast<T*>(m_pCodec))->Decode(m_pRecvBuff, std::forward<Targs>(args)...);
        }
        if (CODEC_STATUS_OK == eCodecStatus || CODEC_STATUS_PART_OK == eCodecStatus)
        {
            ++m_uiUnitTimeMsgNum;
            ++m_uiMsgNum;
            if (m_uiMsgNum == 1)
            {
                m_dKeepAlive = m_pLabor->GetNodeInfo().dIoTimeout;
                m_ucChannelStatus = CHANNEL_STATUS_ESTABLISHED;
            }
        }
        else
        {
            m_pRecvBuff->SetReadIndex(uiReadIndex);
            if (0 == m_uiMsgNum && CODEC_STATUS_PAUSE != eCodecStatus)
            {
                if (!IsClient())
                {
                    m_pSendBuff->Clear();
                }
                return(CODEC_STATUS_INVALID);
            }
        }
        return(eCodecStatus);
    }
    return(m_eLastCodecStatus);
}

template<typename T>
template <typename ...Targs>
E_CODEC_STATUS SocketChannelImpl<T>::Fetch(Targs&&... args)
{
    LOG4_TRACE("channel_fd[%d], channel_seq[%d]", m_iFd, m_uiSeq);
    if (CHANNEL_STATUS_CLOSED == m_ucChannelStatus || CHANNEL_STATUS_BROKEN == m_ucChannelStatus)
    {
        LOG4_TRACE("%s channel_fd[%d], channel_seq[%d], channel_status[%d] remote %s EOF.",
                m_strIdentify.c_str(), m_iFd, m_uiSeq, (int)m_ucChannelStatus, m_strRemoteAddr.c_str());
        return(CODEC_STATUS_ERR);
    }
    auto uiReadIndex = m_pRecvBuff->GetReadIndex();
    E_CODEC_STATUS eCodecStatus = CODEC_STATUS_OK;
    if (m_pCodec->DecodeWithReactor())
    {
        size_t uiSendBuffLen = m_pSendBuff->ReadableBytes();
        eCodecStatus = (static_cast<T*>(m_pCodec))->Decode(m_pRecvBuff, std::forward<Targs>(args)..., m_pSendBuff);
        if (m_pSendBuff->ReadableBytes() > uiSendBuffLen
                && (eCodecStatus == CODEC_STATUS_OK || eCodecStatus == CODEC_STATUS_PART_OK))
        {
            Send();
        }
    }
    else
    {
        eCodecStatus = (static_cast<T*>(m_pCodec))->Decode(m_pRecvBuff, std::forward<Targs>(args)...);
    }
    if (CODEC_STATUS_OK == eCodecStatus || CODEC_STATUS_PART_OK == eCodecStatus)
    {
        ++m_uiUnitTimeMsgNum;
        ++m_uiMsgNum;
        if (m_uiMsgNum == 1)
        {
            m_dKeepAlive = m_pLabor->GetNodeInfo().dIoTimeout;
            m_ucChannelStatus = CHANNEL_STATUS_ESTABLISHED;
        }
    }
    else
    {
        m_pRecvBuff->SetReadIndex(uiReadIndex);
        if (0 == m_uiMsgNum && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            return(CODEC_STATUS_INVALID);
        }
    }
    if ((CODEC_STATUS_PAUSE == eCodecStatus)
            && (CODEC_STATUS_EOF == m_eLastCodecStatus
                || CODEC_STATUS_INT == m_eLastCodecStatus))
    {
        return(m_eLastCodecStatus);
    }
    return(eCodecStatus);
}

template<typename T>
void SocketChannelImpl<T>::SetSecretKey(const std::string& strKey)
{
    m_strKey = strKey;
    m_pCodec->SetKey(m_strKey);
}

template<typename T>
void SocketChannelImpl<T>::SetRemoteWorkerIndex(int16 iRemoteWorkerIndex)
{
    m_iRemoteWorkerIdx = iRemoteWorkerIndex;
}

template<typename T>
bool SocketChannelImpl<T>::Close()
{
    LOG4_TRACE("channel[%d] channel_status %d", m_iFd, (int)m_ucChannelStatus);
    if (m_pCodec != nullptr)
    {
        m_pCodec->UnbindChannel();
    }
    if (CHANNEL_STATUS_CLOSED != m_ucChannelStatus)
    {
        m_pSendBuff->Compact(1);
        m_pWaitForSendBuff->Compact(1);
        if (0 == close(m_iFd))
        {
            m_ucChannelStatus = CHANNEL_STATUS_CLOSED;
            LOG4_TRACE("channel[%d], channel_seq[%u] close successfully.", m_iFd, GetSequence());
            return(true);
        }
        else
        {
            LOG4_TRACE("failed to close channel(fd %d), errno %d, it will be close later.", m_iFd, errno);
            return(false);
        }
    }
    else
    {
        LOG4_TRACE("channel(fd %d, seq %u) had been closed before.", m_iFd, GetSequence());
        return(false);
    }
}

template<typename T>
bool SocketChannelImpl<T>::SetCodec(Codec* pCodec)
{
    if (pCodec == nullptr)
    {
        return(false);
    }
    if (m_pCodec != nullptr)
    {
        delete m_pCodec;
    }
    pCodec->ConnectionSetting(m_pSendBuff);
    m_pCodec = pCodec;
    return(true);
}

template<typename T>
int SocketChannelImpl<T>::Write(CBuffer* pBuff, int& iErrno)
{
    LOG4_TRACE("fd[%d], channel_seq[%u]", GetFd(), GetSequence());
    return(pBuff->WriteFD(m_iFd, iErrno));
}

template<typename T>
int SocketChannelImpl<T>::Read(CBuffer* pBuff, int& iErrno)
{
    LOG4_TRACE("fd[%d], channel_seq[%u]", GetFd(), GetSequence());
    return(pBuff->ReadFD(m_iFd, iErrno));
}

} /* namespace neb */

#endif /* SRC_CHANNEL_SOCKETCHANNELIMPL_HPP_ */

