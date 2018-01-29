/*******************************************************************************
 * Project:  Nebula
 * @file     Worker.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月13日
 * @note
 * Modify history:
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"
#include "util/process_helper.h"
#include "util/proctitle_helper.h"
#ifdef __cplusplus
}
#endif
#include "Worker.hpp"
#include "object/step/Step.hpp"
#include "object/step/RedisStep.hpp"
#include "object/step/HttpStep.hpp"
#include "object/session/Session.hpp"
#include "object/cmd/Cmd.hpp"
#include "object/cmd/Module.hpp"
#include "object/cmd/sys_cmd/CmdConnectWorker.hpp"
#include "object/cmd/sys_cmd/CmdToldWorker.hpp"
#include "object/cmd/sys_cmd/CmdUpdateNodeId.hpp"
#include "object/cmd/sys_cmd/CmdNodeNotice.hpp"
#include "object/cmd/sys_cmd/CmdBeat.hpp"
#include "object/step/sys_step/StepIoTimeout.hpp"

//#include <iostream>

namespace neb
{

void Worker::TerminatedCallback(struct ev_loop* loop, struct ev_signal* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->Terminated(watcher);  // timeout，worker进程无响应或与Manager通信通道异常，被manager进程终止时返回
    }
}

void Worker::IdleCallback(struct ev_loop* loop, struct ev_idle* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)watcher->data;
        pWorker->CheckParent();
    }
}

void Worker::IoCallback(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Channel* pChannel = (Channel*)watcher->data;
        Worker* pWorker = (Worker*)pChannel->m_pLabor;
        if (revents & EV_READ)
        {
            pWorker->IoRead(pChannel);
        }
        if (revents & EV_WRITE)
        {
            pWorker->IoWrite(pChannel);
        }
        if (CHANNEL_STATUS_DISCARD == pChannel->GetChannelStatus() || CHANNEL_STATUS_DESTROY == pChannel->GetChannelStatus())
        {
            DELETE(pChannel);
        }
    }
}

void Worker::IoTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Channel* pChannel = (Channel*)watcher->data;
        Worker* pWorker = (Worker*)(pChannel->m_pLabor);
//        std::cerr << "loop = " << loop << std::endl;
//        std::cerr << "IoTimeoutCallback() watcher = " << watcher << " pChannel = " << pChannel << "  pWorker = " << pWorker << std::endl;
//        std::cerr << "pChannel->GetFd() = " << pChannel->GetFd() << " pChannel->GetSequence() = " << pChannel->GetSequence() << "  pChannel->MutableTimerWatcher() = " << pChannel->MutableTimerWatcher() << std::endl;
        if (pChannel->GetFd() < 3)      // TODO 这个判断是不得已的做法，需查找fd为0回调到这里的原因
        {
            return;
        }
        pWorker->IoTimeout(pChannel);
    }
}

void Worker::PeriodicTaskCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Worker* pWorker = (Worker*)(watcher->data);
//#ifndef NODE_TYPE_BEACON
        pWorker->CheckParent();
//#endif
    }
    ev_timer_stop (loop, watcher);
    ev_timer_set (watcher, NODE_BEAT + ev_time() - ev_now(loop), 0);
    ev_timer_start (loop, watcher);
}

void Worker::StepTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Step* pStep = (Step*)watcher->data;
        ((Worker*)(pStep->m_pLabor))->StepTimeout(pStep);
    }
}

void Worker::SessionTimeoutCallback(struct ev_loop* loop, ev_timer* watcher, int revents)
{
    if (watcher->data != NULL)
    {
        Session* pSession = (Session*)watcher->data;
        ((Worker*)pSession->m_pLabor)->SessionTimeout(pSession);
    }
}

void Worker::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->RedisConnect(c, status);
    }
}

void Worker::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->RedisDisconnect(c, status);
    }
}

void Worker::RedisCmdCallback(redisAsyncContext *c, void *reply, void *privdata)
{
    if (c->data != NULL)
    {
        Worker* pWorker = (Worker*)c->data;
        pWorker->RedisCmdResult(c, reply, privdata);
    }
}

Worker::Worker(const std::string& strWorkPath, int iControlFd, int iDataFd, int iWorkerIndex, CJsonObject& oJsonConf)
    : m_pErrBuff(NULL), m_ulSequence(0), m_bInitLogger(false), m_dIoTimeout(480.0), m_strWorkPath(strWorkPath), m_uiNodeId(0),
      m_iManagerControlFd(iControlFd), m_iManagerDataFd(iDataFd), m_iWorkerIndex(iWorkerIndex), m_iWorkerPid(0),
      m_dMsgStatInterval(60.0), m_iMsgPermitNum(60),
      m_iRecvNum(0), m_iRecvByte(0), m_iSendNum(0), m_iSendByte(0),
      m_loop(NULL), m_pCmdConnect(NULL)
{
    m_iWorkerPid = getpid();
    m_pErrBuff = (char*)malloc(gc_iErrBuffLen);
    if (!Init(oJsonConf))
    {
        exit(-1);
    }
    if (!CreateEvents())
    {
        exit(-2);
    }
    PreloadCmd();
    LoadCmd(oJsonConf["cmd"]);
    LoadModule(oJsonConf["module"]);
}

Worker::~Worker()
{
    Destroy();
}

void Worker::Run()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_run (m_loop, 0);
}

void Worker::Terminated(struct ev_signal* watcher)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iSignum = watcher->signum;
    delete watcher;
    //Destroy();
    LOG4_FATAL("terminated by signal %d!", iSignum);
    exit(iSignum);
}

bool Worker::CheckParent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    pid_t iParentPid = getppid();
    if (iParentPid == 1)    // manager进程已不存在
    {
        LOG4_INFO("no manager process exist, worker %d exit.", m_iWorkerIndex);
        //Destroy();
        exit(0);
    }
    MsgBody oMsgBody;
    CJsonObject oJsonLoad;
    oJsonLoad.Add("load", int32(m_mapChannel.size() + m_mapCallbackStep.size()));
    oJsonLoad.Add("connect", int32(m_mapChannel.size()));
    oJsonLoad.Add("recv_num", m_iRecvNum);
    oJsonLoad.Add("recv_byte", m_iRecvByte);
    oJsonLoad.Add("send_num", m_iSendNum);
    oJsonLoad.Add("send_byte", m_iSendByte);
    oJsonLoad.Add("client", int32(m_mapChannel.size() - m_mapInnerFd.size()));
    oMsgBody.set_data(oJsonLoad.ToString());
    LOG4_TRACE("%s", oJsonLoad.ToString().c_str());
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(m_iManagerControlFd);
    if (iter != m_mapChannel.end())
    {
        iter->second->Send(CMD_REQ_UPDATE_WORKER_LOAD, GetSequence(), oMsgBody);
    }
    m_iRecvNum = 0;
    m_iRecvByte = 0;
    m_iSendNum = 0;
    m_iSendByte = 0;
    return(true);
}

bool Worker::IoRead(Channel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pChannel->GetFd() == m_iManagerDataFd)
    {
        return(FdTransfer());
    }
    else
    {
        return(RecvDataAndHandle(pChannel));
    }
}

bool Worker::RecvDataAndHandle(Channel* pChannel)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    E_CODEC_STATUS eCodecStatus;
    if (CODEC_HTTP == pChannel->GetCodecType())
    {
        for (int i = 0; ; ++i)
        {
            HttpMsg oHttpMsg;
            if (0 == i)
            {
                eCodecStatus = pChannel->Recv(oHttpMsg);
            }
            else
            {
                eCodecStatus = pChannel->Fetch(oHttpMsg);
            }

            if (CODEC_STATUS_OK == eCodecStatus)
            {
                Handle(pChannel, oHttpMsg);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        eCodecStatus = pChannel->Recv(oMsgHead, oMsgBody);
        while (CODEC_STATUS_OK == eCodecStatus)
        {
#ifdef NODE_TYPE_ACCESS
            if (!pChannel->IsChannelVerify())
            {
                std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(pChannel->GetFd());
                if (inner_iter == m_mapInnerFd.end() && pChannel->GetMsgNum() > 1)   // 未经账号验证的客户端连接发送数据过来，直接断开
                {
                    LOG4_DEBUG("invalid request, please login first!");
                    DiscardChannel(pChannel);
                    return(false);
                }
            }
#endif
            Handle(pChannel, oMsgHead, oMsgBody);
            oMsgHead.Clear();       // 注意protobuf的Clear()使用不当容易造成内存泄露
            oMsgBody.Clear();
            eCodecStatus = pChannel->Fetch(oMsgHead, oMsgBody);
        }
    }

    if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        return(true);
    }
    else    // 编解码出错或连接关闭或连接中断
    {
        DiscardChannel(pChannel);
        return(false);
    }
}

bool Worker::FdTransfer()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    char szIpAddr[16] = {0};
    int iCodec = 0;
    // int iAcceptFd = recv_fd(m_iManagerDataFd);
    int iAcceptFd = recv_fd_with_attr(m_iManagerDataFd, szIpAddr, 16, &iCodec);
    if (iAcceptFd <= 0)
    {
        if (iAcceptFd == 0)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d len %d", m_iManagerDataFd, iAcceptFd);
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
        else if (errno != EAGAIN)
        {
            LOG4_ERROR("recv_fd from m_iManagerDataFd %d error %d", m_iManagerDataFd, errno);
            //Destroy();
            exit(2); // manager与worker通信fd已关闭，worker进程退出
        }
    }
    else
    {
        Channel* pChannel = CreateChannel(iAcceptFd, E_CODEC_TYPE(iCodec));
        if (NULL != pChannel)
        {
            int z;                          /* status return code */
            struct sockaddr_in adr_inet;    /* AF_INET */
            unsigned int len_inet;                   /* length */
            z = getpeername(iAcceptFd, (struct sockaddr *)&adr_inet, &len_inet);
            pChannel->SetRemoteAddr(inet_ntoa(adr_inet.sin_addr));
            AddIoTimeout(pChannel, 1.0);     // 为了防止大量连接攻击，初始化连接只有一秒即超时，在正常发送第一个数据包之后才采用正常配置的网络IO超时检查
            AddIoReadEvent(pChannel);
            if (CODEC_PROTOBUF != iCodec)
            {
                pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
            }
        }
        else    // 没有足够资源分配给新连接，直接close掉
        {
            close(iAcceptFd);
        }
    }
    return(false);
}

bool Worker::IoWrite(Channel* pChannel)
{
    //LOG4_TRACE("%s()", __FUNCTION__);
    if (CHANNEL_STATUS_TRY_CONNECT == pChannel->GetChannelStatus())  // connect之后的第一个写事件
    {
        std::map<uint32, int>::iterator index_iter = m_mapSeq2WorkerIndex.find(pChannel->GetSequence());
        if (index_iter != m_mapSeq2WorkerIndex.end())
        {
            tagChannelContext stCtx;
            stCtx.iFd = pChannel->GetFd();
            stCtx.ulSeq = pChannel->GetSequence();
            AddInnerChannel(stCtx);
            if (CODEC_PROTOBUF == pChannel->GetCodecType())  // 系统内部Server间通信
            {
                ((CmdConnectWorker*)m_pCmdConnect)->Start(stCtx, index_iter->second);
            }
            m_mapSeq2WorkerIndex.erase(index_iter);
            return(false);
        }
    }

    if (CODEC_PROTOBUF != pChannel->GetCodecType()
            && CHANNEL_STATUS_ESTABLISHED != pChannel->GetChannelStatus())
    {
        pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    }

    E_CODEC_STATUS eCodecStatus = pChannel->Send();
    if (CODEC_STATUS_OK == eCodecStatus)
    {
        RemoveIoWriteEvent(pChannel);
    }
    else if (CODEC_STATUS_PAUSE == eCodecStatus)
    {
        AddIoWriteEvent(pChannel);
    }
    else
    {
        DiscardChannel(pChannel);
    }
    return(true);
}

bool Worker::IoTimeout(Channel* pChannel)
{
    ev_tstamp after = pChannel->GetActiveTime() - ev_now(m_loop) + m_dIoTimeout;
    if (after > 0)    // IO在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, pChannel->MutableTimerWatcher());
        ev_timer_set (pChannel->MutableTimerWatcher(), after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, pChannel->MutableTimerWatcher());
        return(true);
    }

    LOG4_TRACE("%s(fd %d, seq %u):", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    if (pChannel->NeedAliveCheck())     // 需要发送心跳检查
    {
        tagChannelContext stCtx;
        stCtx.iFd = pChannel->GetFd();
        stCtx.ulSeq = pChannel->GetSequence();
        StepIoTimeout* pStepIoTimeout = NULL;
        try
        {
            pStepIoTimeout = new StepIoTimeout(stCtx);
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new StepIoTimeout error: %s", e.what());
            DiscardChannel(pChannel);
            return(false);
        }
        if (Register((Step*)pStepIoTimeout))
        {
            E_CMD_STATUS eStatus = pStepIoTimeout->Emit(ERR_OK);
            if (CMD_STATUS_RUNNING != eStatus)
            {
                // 若返回非running状态，则表明发包时已出错，
                // 销毁连接过程在SentTo里已经完成，这里不需要再销毁连接
                Remove(pStepIoTimeout);
            }
            else
            {
                return(true);
            }
        }
        else
        {
            DELETE(pStepIoTimeout);
            DiscardChannel(pChannel);
        }
    }
    else        // 关闭文件描述符并清理相关资源
    {
        std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(pChannel->GetFd());
        if (inner_iter == m_mapInnerFd.end())   // 非内部服务器间的连接才会在超时中关闭
        {
            LOG4_TRACE("io timeout!");
            DiscardChannel(pChannel);
        }
    }
    return(true);
}

bool Worker::StepTimeout(Step* pStep)
{
    ev_timer* watcher = pStep->MutableTimerWatcher();
    ev_tstamp after = pStep->GetActiveTime() - ev_now(m_loop) + pStep->GetTimeout();
    if (after > 0)    // 在定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 步骤已超时
    {
        LOG4_TRACE("%s(seq %lu): active_time %lf, now_time %lf, lifetime %lf",
                        __FUNCTION__, pStep->GetSequence(), pStep->GetActiveTime(), ev_now(m_loop), pStep->GetTimeout());
        E_CMD_STATUS eResult = pStep->Timeout();
        if (CMD_STATUS_RUNNING == eResult)
        {
            ev_tstamp after = pStep->GetTimeout();
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            Remove(pStep);
            return(true);
        }
    }
}

bool Worker::SessionTimeout(Session* pSession)
{
    ev_timer* watcher = pSession->MutableTimerWatcher();
    ev_tstamp after = pSession->GetActiveTime() - ev_now(m_loop) + pSession->GetTimeout();
    if (after > 0)    // 定时时间内被重新刷新过，重新设置定时器
    {
        ev_timer_stop (m_loop, watcher);
        ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, watcher);
        return(true);
    }
    else    // 会话已超时
    {
        LOG4_TRACE("%s(session_name: %s,  session_id: %s)",
                        __FUNCTION__, pSession->GetSessionClass().c_str(), pSession->GetSessionId().c_str());
        if (CMD_STATUS_RUNNING == pSession->Timeout())
        {
            ev_timer_stop (m_loop, watcher);
            ev_timer_set (watcher, pSession->GetTimeout() + ev_time() - ev_now(m_loop), 0);
            ev_timer_start (m_loop, watcher);
            return(true);
        }
        else
        {
            Remove(pSession);
            return(true);
        }
    }
}

bool Worker::RedisConnect(const redisAsyncContext *c, int status)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        if (status == REDIS_OK)
        {
            attr_iter->second->bIsReady = true;
            int iCmdStatus;
            for (std::list<RedisStep*>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); )
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
                const char* argv[args_size];
                size_t arglen[args_size];
                argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
                arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
                std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
                for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
                {
                    argv[i] = c_iter->first.c_str();
                    arglen[i] = c_iter->first.size();
                }
                iCmdStatus = redisAsyncCommandArgv((redisAsyncContext*)c, RedisCmdCallback, NULL, args_size, argv, arglen);
                if (iCmdStatus == REDIS_OK)
                {
                    LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                    attr_iter->second->listData.push_back(pRedisStep);
                    attr_iter->second->listWaitData.erase(step_iter++);
                }
                else    // 命令执行失败，不再继续执行，等待下一次回调
                {
                    break;
                }
            }
        }
        else
        {
            for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                            step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
            {
                RedisStep* pRedisStep = (RedisStep*)(*step_iter);
                pRedisStep->Callback(c, status, NULL);
                delete pRedisStep;
            }
            attr_iter->second->listWaitData.clear();
            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
    }
    return(true);
}

bool Worker::RedisDisconnect(const redisAsyncContext *c, int status)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listData.begin();
                        step_iter != attr_iter->second->listData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listData.clear();

        for (std::list<RedisStep *>::iterator step_iter = attr_iter->second->listWaitData.begin();
                        step_iter != attr_iter->second->listWaitData.end(); ++step_iter)
        {
            LOG4_ERROR("RedisDisconnect callback error %d of redis cmd: %s",
                            c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
            (*step_iter)->Callback(c, c->err, NULL);
            delete (*step_iter);
            (*step_iter) = NULL;
        }
        attr_iter->second->listWaitData.clear();

        delete attr_iter->second;
        attr_iter->second = NULL;
        DelRedisContextAddr(c);
        m_mapRedisAttr.erase(attr_iter);
    }
    return(true);
}

bool Worker::RedisCmdResult(redisAsyncContext *c, void *reply, void *privdata)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<redisAsyncContext*, tagRedisAttr*>::iterator attr_iter = m_mapRedisAttr.find((redisAsyncContext*)c);
    if (attr_iter != m_mapRedisAttr.end())
    {
        std::list<RedisStep*>::iterator step_iter = attr_iter->second->listData.begin();
        if (NULL == reply)
        {
            std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(c);
            if (identify_iter != m_mapContextIdentify.end())
            {
                LOG4_ERROR("redis %s error %d: %s", identify_iter->second.c_str(), c->err, c->errstr);
            }
            for ( ; step_iter != attr_iter->second->listData.end(); ++step_iter)
            {
                LOG4_ERROR("callback error %d of redis cmd: %s", c->err, (*step_iter)->GetRedisCmd()->ToString().c_str());
                (*step_iter)->Callback(c, c->err, (redisReply*)reply);
                delete (*step_iter);
                (*step_iter) = NULL;
            }
            attr_iter->second->listData.clear();

            delete attr_iter->second;
            attr_iter->second = NULL;
            DelRedisContextAddr(c);
            m_mapRedisAttr.erase(attr_iter);
        }
        else
        {
            if (step_iter != attr_iter->second->listData.end())
            {
                LOG4_TRACE("callback of redis cmd: %s", (*step_iter)->GetRedisCmd()->ToString().c_str());
                /** @note 注意，若Callback返回STATUS_CMD_RUNNING，框架不回收并且不再管理该RedisStep，该RedisStep后续必须重新RegisterCallback或由开发者自己回收 */
                if (CMD_STATUS_RUNNING != (*step_iter)->Callback(c, REDIS_OK, (redisReply*)reply))
                {
                    delete (*step_iter);
                    (*step_iter) = NULL;
                }
                attr_iter->second->listData.erase(step_iter);
                //freeReplyObject(reply);
            }
            else
            {
                LOG4_ERROR("no redis callback data found!");
            }
        }
    }
    return(true);
}

time_t Worker::GetNowTime() const
{
    return((time_t)ev_now(m_loop));
}

bool Worker::Pretreat(Cmd* pCmd)
{
    LOG4_TRACE("%s(Cmd*)", __FUNCTION__);
    if (pCmd == NULL)
    {
        return(false);
    }
    pCmd->SetLabor(this);
    pCmd->SetLogger(&m_oLogger);
    return(true);
}

bool Worker::Pretreat(Step* pStep)
{
    LOG4_TRACE("%s(Step*)", __FUNCTION__);
    if (pStep == NULL)
    {
        return(false);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    return(true);
}

bool Worker::Pretreat(Session* pSession)
{
    LOG4_TRACE("%s(Session*)", __FUNCTION__);
    if (pSession == NULL)
    {
        return(false);
    }
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    return(true);
}

bool Worker::Register(Step* pStep, ev_tstamp dTimeout)
{
    ev_tstamp dLifeTime = (dTimeout == 0) ? m_dStepTimeout : dTimeout;
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dLifeTime);
    if (pStep == NULL)
    {
        return(false);
    }
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pStep->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }

    std::pair<std::map<uint32, Step*>::iterator, bool> ret
        = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        ev_timer_init (timer_watcher, StepTimeoutCallback, dLifeTime + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStep->GetSequence(), pStep->GetActiveTime(), pStep->GetTimeout());
    }
    return(ret.second);
}

bool Worker::Register(uint32 uiSelfStepSeq, Step* pStep, ev_tstamp dTimeout)
{
    ev_tstamp dLifeTime = (dTimeout == 0) ? m_dStepTimeout : dTimeout;
    LOG4_TRACE("%s(Step* 0x%X, lifetime %lf)", __FUNCTION__, pStep, dLifeTime);
    if (pStep == NULL)
    {
        return(false);
    }
    std::map<uint32, Step*>::iterator callback_iter;
    std::set<uint32>::iterator next_step_seq_iter;
    if (pStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        // 登记前置step
        next_step_seq_iter = pStep->m_setNextStepSeq.find(uiSelfStepSeq);
        if (next_step_seq_iter != pStep->m_setNextStepSeq.end())
        {
            callback_iter = m_mapCallbackStep.find(uiSelfStepSeq);
            if (callback_iter != m_mapCallbackStep.end())
            {
                callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
            }
        }
        return(true);
    }
    pStep->SetLabor(this);
    pStep->SetLogger(&m_oLogger);
    pStep->SetRegistered();
    pStep->SetActiveTime(ev_now(m_loop));

    // 登记前置step
    next_step_seq_iter = pStep->m_setNextStepSeq.find(uiSelfStepSeq);
    if (next_step_seq_iter != pStep->m_setNextStepSeq.end())
    {
        callback_iter = m_mapCallbackStep.find(uiSelfStepSeq);
        if (callback_iter != m_mapCallbackStep.end())
        {
            callback_iter->second->m_setPreStepSeq.insert(pStep->GetSequence());
        }
    }

    ev_timer* timer_watcher = pStep->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }

    std::pair<std::map<uint32, Step*>::iterator, bool> ret
        = m_mapCallbackStep.insert(std::pair<uint32, Step*>(pStep->GetSequence(), pStep));
    if (ret.second)
    {
        ev_timer_init (timer_watcher, StepTimeoutCallback, dLifeTime + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        LOG4_TRACE("Step(seq %u, active_time %lf, lifetime %lf) register successful.",
                        pStep->GetSequence(), pStep->GetActiveTime(), pStep->GetTimeout());
    }
    return(ret.second);
}

void Worker::Remove(Step* pStep)
{
    LOG4_TRACE("%s(Step* 0x%X)", __FUNCTION__, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::map<uint32, Step*>::iterator callback_iter;
    for (std::set<uint32>::iterator step_seq_iter = pStep->m_setPreStepSeq.begin();
                    step_seq_iter != pStep->m_setPreStepSeq.end(); )
    {
        callback_iter = m_mapCallbackStep.find(*step_seq_iter);
        if (callback_iter == m_mapCallbackStep.end())
        {
            pStep->m_setPreStepSeq.erase(step_seq_iter++);
        }
        else
        {
            LOG4_DEBUG("step %u had pre step %u running, delay delete callback.", pStep->GetSequence(), *step_seq_iter);
            pStep->DelayTimeout();
            return;
        }
    }
    if (pStep->MutableTimerWatcher() != NULL)
    {
        ev_timer_stop (m_loop, pStep->MutableTimerWatcher());
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("delete step(seq %u)", pStep->GetSequence());
        DELETE(pStep);
        m_mapCallbackStep.erase(callback_iter);
    }
}

void Worker::Remove(uint32 uiSelfStepSeq, Step* pStep)
{
    LOG4_TRACE("%s(self_seq[%u], Step* 0x%X)", __FUNCTION__, uiSelfStepSeq, pStep);
    if (pStep == NULL)
    {
        return;
    }
    std::map<uint32, Step*>::iterator callback_iter;
    for (std::set<uint32>::iterator step_seq_iter = pStep->m_setPreStepSeq.begin();
                    step_seq_iter != pStep->m_setPreStepSeq.end(); )
    {
        callback_iter = m_mapCallbackStep.find(*step_seq_iter);
        if (callback_iter == m_mapCallbackStep.end())
        {
            LOG4_TRACE("try to erase seq[%u] from pStep->m_setPreStepSeq", *step_seq_iter);
            pStep->m_setPreStepSeq.erase(step_seq_iter++);
        }
        else
        {
            if (*step_seq_iter != uiSelfStepSeq)
            {
                LOG4_DEBUG("step[%u] try to delete step[%u], but step[%u] had pre step[%u] running, delay delete callback.",
                                uiSelfStepSeq, pStep->GetSequence(), pStep->GetSequence(), *step_seq_iter);
                pStep->DelayTimeout();
                return;
            }
            else
            {
                step_seq_iter++;
            }
        }
    }
    if (pStep->MutableTimerWatcher() != NULL)
    {
        ev_timer_stop (m_loop, pStep->MutableTimerWatcher());
    }
    callback_iter = m_mapCallbackStep.find(pStep->GetSequence());
    if (callback_iter != m_mapCallbackStep.end())
    {
        LOG4_TRACE("step[%u] try to delete step[%u]", uiSelfStepSeq, pStep->GetSequence());
        DELETE(pStep);
        m_mapCallbackStep.erase(callback_iter);
    }
}

bool Worker::Register(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X, lifetime %lf)", __FUNCTION__, &pSession, pSession->GetTimeout());
    if (pSession == NULL)
    {
        return(false);
    }
    if (pSession->IsRegistered())  // 已注册过，不必重复注册
    {
        return(true);
    }
    pSession->SetLabor(this);
    pSession->SetLogger(&m_oLogger);
    pSession->SetRegistered();
    pSession->SetActiveTime(ev_now(m_loop));
    ev_timer* timer_watcher = pSession->AddTimerWatcher();
    if (NULL == timer_watcher)
    {
        return(false);
    }

    std::pair<std::map<std::string, Session*>::iterator, bool> ret;
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter == m_mapCallbackSession.end())
    {
        std::map<std::string, Session*> mapSession;
        ret = mapSession.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
        m_mapCallbackSession.insert(std::pair<std::string, std::map<std::string, Session*> >(pSession->GetSessionClass(), mapSession));
    }
    else
    {
        ret = name_iter->second.insert(std::pair<std::string, Session*>(pSession->GetSessionId(), pSession));
    }
    if (ret.second)
    {
        if (pSession->GetTimeout() != 0)
        {
            ev_timer_init (timer_watcher, SessionTimeoutCallback, pSession->GetTimeout() + ev_time() - ev_now(m_loop), 0.0);
            ev_timer_start (m_loop, timer_watcher);
        }
        LOG4_TRACE("Session(session_id %s) register successful.", pSession->GetSessionId().c_str());
    }
    return(ret.second);
}

void Worker::Remove(Session* pSession)
{
    LOG4_TRACE("%s(Session* 0x%X)", __FUNCTION__, &pSession);
    if (pSession == NULL)
    {
        return;
    }
    if (pSession->MutableTimerWatcher() != NULL)
    {
        ev_timer_stop (m_loop, pSession->MutableTimerWatcher());
    }
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(pSession->GetSessionClass());
    if (name_iter != m_mapCallbackSession.end())
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(pSession->GetSessionId());
        if (id_iter != name_iter->second.end())
        {
            LOG4_TRACE("delete session(session_id %s)", pSession->GetSessionId().c_str());
            DELETE(id_iter->second);
            name_iter->second.erase(id_iter);
        }
    }
}

bool Worker::Register(const redisAsyncContext* pRedisContext, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    if (pRedisStep == NULL)
    {
        return(false);
    }
    if (pRedisStep->IsRegistered())  // 已注册过，不必重复注册，不过认为本次注册成功
    {
        return(true);
    }
    pRedisStep->SetLabor(this);
    pRedisStep->SetLogger(&m_oLogger);
    pRedisStep->SetRegistered();

    std::map<redisAsyncContext*, tagRedisAttr*>::iterator iter = m_mapRedisAttr.find((redisAsyncContext*)pRedisContext);
    if (iter == m_mapRedisAttr.end())
    {
        LOG4_ERROR("redis attr not exist!");
        return(false);
    }
    else
    {
        LOG4_TRACE("iter->second->bIsReady = %d", iter->second->bIsReady);
        if (iter->second->bIsReady)
        {
            int status;
            size_t args_size = pRedisStep->GetRedisCmd()->GetCmdArguments().size() + 1;
            const char* argv[args_size];
            size_t arglen[args_size];
            argv[0] = pRedisStep->GetRedisCmd()->GetCmd().c_str();
            arglen[0] = pRedisStep->GetRedisCmd()->GetCmd().size();
            std::vector<std::pair<std::string, bool> >::const_iterator c_iter = pRedisStep->GetRedisCmd()->GetCmdArguments().begin();
            for (size_t i = 1; c_iter != pRedisStep->GetRedisCmd()->GetCmdArguments().end(); ++c_iter, ++i)
            {
                argv[i] = c_iter->first.c_str();
                arglen[i] = c_iter->first.size();
            }
            status = redisAsyncCommandArgv((redisAsyncContext*)pRedisContext, RedisCmdCallback, NULL, args_size, argv, arglen);
            if (status == REDIS_OK)
            {
                LOG4_DEBUG("succeed in sending redis cmd: %s", pRedisStep->GetRedisCmd()->ToString().c_str());
                iter->second->listData.push_back(pRedisStep);
                return(true);
            }
            else
            {
                LOG4_ERROR("redis status %d!", status);
                return(false);
            }
        }
        else
        {
            LOG4_TRACE("listWaitData.push_back()");
            iter->second->listWaitData.push_back(pRedisStep);
            return(true);
        }
    }
}

bool Worker::ResetTimeout(Object* pObject)
{
    ev_timer* watcher = pObject->MutableTimerWatcher();
    ev_tstamp after = ev_now(m_loop) + pObject->GetTimeout();
    ev_timer_stop (m_loop, watcher);
    ev_timer_set (watcher, after + ev_time() - ev_now(m_loop), 0);
    ev_timer_start (m_loop, watcher);
    return(true);
}

Session* Worker::GetSession(uint32 uiSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        char szSession[16] = {0};
        snprintf(szSession, sizeof(szSession), "%u", uiSessionId);
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(szSession);
        if (id_iter == name_iter->second.end())
        {
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

Session* Worker::GetSession(const std::string& strSessionId, const std::string& strSessionClass)
{
    std::map<std::string, std::map<std::string, Session*> >::iterator name_iter = m_mapCallbackSession.find(strSessionClass);
    if (name_iter == m_mapCallbackSession.end())
    {
        return(NULL);
    }
    else
    {
        std::map<std::string, Session*>::iterator id_iter = name_iter->second.find(strSessionId);
        if (id_iter == name_iter->second.end())
        {
            return(NULL);
        }
        else
        {
            id_iter->second->SetActiveTime(ev_now(m_loop));
            return(id_iter->second);
        }
    }
}

bool Worker::Disconnect(const tagChannelContext& stCtx, bool bChannelNotice)
{
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter != m_mapChannel.end())
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            LOG4_TRACE("if (iter->second->ulSeq == stCtx.ulSeq)");
            return(DiscardChannel(iter->second, bChannelNotice));
        }
    }
    return(false);
}

bool Worker::Disconnect(const std::string& strIdentify, bool bChannelNotice)
{
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
    if (named_iter != m_mapNamedChannel.end())
    {
        std::list<Channel*>::iterator channel_iter;
        while (named_iter->second.size() > 1)
        {
            channel_iter = named_iter->second.begin();
            DiscardChannel(*channel_iter, bChannelNotice);
        }
        channel_iter = named_iter->second.begin();
        return(DiscardChannel(*channel_iter, bChannelNotice));
    }
    return(false);
}

bool Worker::SetProcessName(const CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    return(true);
}

bool Worker::Init(CJsonObject& oJsonConf)
{
    char szProcessName[64] = {0};
    snprintf(szProcessName, sizeof(szProcessName), "%s_W%d", oJsonConf("server_name").c_str(), m_iWorkerIndex);
    ngx_setproctitle(szProcessName);
    oJsonConf.Get("io_timeout", m_dIoTimeout);
    if (!oJsonConf.Get("step_timeout", m_dStepTimeout))
    {
        m_dStepTimeout = 0.5;
    }
    oJsonConf.Get("node_type", m_strNodeType);
    oJsonConf.Get("host", m_strHostForServer);
    oJsonConf.Get("port", m_iPortForServer);
    m_oCustomConf = oJsonConf["custom"];
#ifdef NODE_TYPE_ACCESS
    oJsonConf["permission"]["uin_permit"].Get("stat_interval", m_dMsgStatInterval);
    oJsonConf["permission"]["uin_permit"].Get("permit_num", m_iMsgPermitNum);
#endif
    if (!InitLogger(oJsonConf))
    {
        return(false);
    }
    m_pCmdConnect = new CmdConnectWorker();
    if (m_pCmdConnect == NULL)
    {
        return(false);
    }
    m_pCmdConnect->SetLogger(&m_oLogger);
    m_pCmdConnect->SetLabor(this);
    return(true);
}

bool Worker::InitLogger(const CJsonObject& oJsonConf)
{
    if (m_bInitLogger)  // 已经被初始化过，只修改日志级别
    {
        int32 iLogLevel = 0;
        oJsonConf.Get("log_level", iLogLevel);
        m_oLogger.setLogLevel(iLogLevel);
        return(true);
    }
    else
    {
        int32 iMaxLogFileSize = 0;
        int32 iMaxLogFileNum = 0;
        int32 iLogLevel = 0;
        int32 iLoggingPort = 9000;
        std::string strLoggingHost;
        std::string strLogname = m_strWorkPath + std::string("/") + oJsonConf("log_path")
                        + std::string("/") + getproctitle() + std::string(".log");
        std::string strParttern = "[%D,%d{%q}][%p] [%l] %m%n";
        std::ostringstream ssServerName;
        ssServerName << getproctitle() << " " << GetWorkerIdentify();
        oJsonConf.Get("max_log_file_size", iMaxLogFileSize);
        oJsonConf.Get("max_log_file_num", iMaxLogFileNum);
        if (oJsonConf.Get("log_level", iLogLevel))
        {
            switch (iLogLevel)
            {
                case log4cplus::DEBUG_LOG_LEVEL:
                    break;
                case log4cplus::INFO_LOG_LEVEL:
                    break;
                case log4cplus::TRACE_LOG_LEVEL:
                    break;
                case log4cplus::WARN_LOG_LEVEL:
                    break;
                case log4cplus::ERROR_LOG_LEVEL:
                    break;
                case log4cplus::FATAL_LOG_LEVEL:
                    break;
                default:
                    iLogLevel = log4cplus::INFO_LOG_LEVEL;
            }
        }
        else
        {
            iLogLevel = log4cplus::INFO_LOG_LEVEL;
        }
        log4cplus::initialize();
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(strParttern));
        log4cplus::SharedAppenderPtr append(new log4cplus::RollingFileAppender(
                        strLogname, iMaxLogFileSize, iMaxLogFileNum));
        append->setName(strLogname);
        append->setLayout(layout);
        m_oLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(strLogname));
        m_oLogger.setLogLevel(iLogLevel);
        m_oLogger.addAppender(append);
        if (oJsonConf.Get("socket_logging_host", strLoggingHost) && oJsonConf.Get("socket_logging_port", iLoggingPort))
        {
            log4cplus::SharedAppenderPtr socket_append(new log4cplus::SocketAppender(
                            strLoggingHost, iLoggingPort, ssServerName.str()));
            socket_append->setName(ssServerName.str());
            socket_append->setLayout(layout);
            socket_append->setThreshold(log4cplus::INFO_LOG_LEVEL);
            m_oLogger.addAppender(socket_append);
        }
        LOG4_INFO("%s program begin...", getproctitle());
        m_bInitLogger = true;
        return(true);
    }
}

bool Worker::CreateEvents()
{
    m_loop = ev_loop_new(EVFLAG_AUTO);
    if (m_loop == NULL)
    {
        return(false);
    }

    signal(SIGPIPE, SIG_IGN);
    // 注册信号事件
    ev_signal* signal_watcher = new ev_signal();
    ev_signal_init (signal_watcher, TerminatedCallback, SIGINT);
    signal_watcher->data = (void*)this;
    ev_signal_start (m_loop, signal_watcher);

    AddPeriodicTaskEvent();
    // 注册闲时处理事件   暂时用定时器实现
//    ev_idle* idle_watcher = new ev_idle();
//    ev_idle_init (idle_watcher, IdleCallback);
//    idle_watcher->data = (void*)this;
//    ev_idle_start (m_loop, idle_watcher);

    // 注册网络IO事件
    Channel* pChannelData = CreateChannel(m_iManagerDataFd, CODEC_PROTOBUF);
    Channel* pChannelControl = CreateChannel(m_iManagerControlFd, CODEC_PROTOBUF);
    if (NULL == pChannelData || NULL == pChannelControl)
    {
        return(false);
    }
    pChannelData->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    pChannelControl->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED);
    AddIoReadEvent(pChannelData);
    // AddIoTimeout(pChannelData, 60.0);   // TODO 需要优化
    AddIoReadEvent(pChannelControl);
    // AddIoTimeout(pChannelControl, 60.0); //TODO 需要优化
    m_mapInnerFd.insert(std::make_pair(m_iManagerDataFd, pChannelData->GetSequence()));
    m_mapInnerFd.insert(std::make_pair(m_iManagerControlFd, pChannelControl->GetSequence()));
    return(true);
}

void Worker::PreloadCmd()
{
    Cmd* pCmdToldWorker = new CmdToldWorker();
    pCmdToldWorker->SetCmd(CMD_REQ_TELL_WORKER);
    pCmdToldWorker->SetLogger(&m_oLogger);
    pCmdToldWorker->SetLabor(this);
    m_mapPreloadCmd.insert(std::pair<int32, Cmd*>(pCmdToldWorker->GetCmd(), pCmdToldWorker));

    Cmd* pCmdUpdateNodeId = new CmdUpdateNodeId();
    pCmdUpdateNodeId->SetCmd(CMD_REQ_REFRESH_NODE_ID);
    pCmdUpdateNodeId->SetLogger(&m_oLogger);
    pCmdUpdateNodeId->SetLabor(this);
    m_mapPreloadCmd.insert(std::pair<int32, Cmd*>(pCmdUpdateNodeId->GetCmd(), pCmdUpdateNodeId));

    Cmd* pCmdNodeNotice = new CmdNodeNotice();
    pCmdNodeNotice->SetCmd(CMD_REQ_NODE_REG_NOTICE);
    pCmdNodeNotice->SetLogger(&m_oLogger);
    pCmdNodeNotice->SetLabor(this);
    m_mapPreloadCmd.insert(std::pair<int32, Cmd*>(pCmdNodeNotice->GetCmd(), pCmdNodeNotice));

    Cmd* pCmdBeat = new CmdBeat();
    pCmdBeat->SetCmd(CMD_REQ_BEAT);
    pCmdBeat->SetLogger(&m_oLogger);
    pCmdBeat->SetLabor(this);
    m_mapPreloadCmd.insert(std::pair<int32, Cmd*>(pCmdBeat->GetCmd(), pCmdBeat));
}

void Worker::Destroy()
{
    LOG4_TRACE("%s()", __FUNCTION__);

    for (std::map<int32, Cmd*>::iterator cmd_iter = m_mapPreloadCmd.begin();
                    cmd_iter != m_mapPreloadCmd.end(); ++cmd_iter)
    {
        DELETE(cmd_iter->second);
    }
    m_mapPreloadCmd.clear();

    for (std::map<int, tagSo*>::iterator so_iter = m_mapCmd.begin();
                    so_iter != m_mapCmd.end(); ++so_iter)
    {
        DELETE(so_iter->second);
    }
    m_mapCmd.clear();

    for (std::map<std::string, tagModule*>::iterator module_iter = m_mapModule.begin();
                    module_iter != m_mapModule.end(); ++module_iter)
    {
        DELETE(module_iter->second);
    }
    m_mapModule.clear();

    for (std::map<int, Channel*>::iterator attr_iter = m_mapChannel.begin();
                    attr_iter != m_mapChannel.end(); ++attr_iter)
    {
        LOG4_TRACE("for (std::map<int, tagConnectionAttr*>::iterator attr_iter = m_mapChannel.begin();");
        DiscardChannel(attr_iter->second);
    }
    m_mapChannel.clear();

    for (std::map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.begin();
            step_iter != m_mapCallbackStep.end(); ++step_iter)
    {
        Remove(step_iter->second);
    }
    m_mapCallbackStep.clear();

    // TODO 待补充完整

    if (m_loop != NULL)
    {
        ev_loop_destroy(m_loop);
        m_loop = NULL;
    }

    if (m_pErrBuff != NULL)
    {
        delete[] m_pErrBuff;
        m_pErrBuff = NULL;
    }
}

void Worker::ResetLogLevel(log4cplus::LogLevel iLogLevel)
{
    m_oLogger.setLogLevel(iLogLevel);
}

bool Worker::AddNamedChannel(const std::string& strIdentify, const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(%s, fd %d, seq %u)", __FUNCTION__, strIdentify.c_str(), stCtx.iFd, stCtx.ulSeq);
    std::map<int, Channel*>::iterator channel_iter = m_mapChannel.find(stCtx.iFd);
    if (channel_iter == m_mapChannel.end())
    {
        return(false);
    }
    else
    {
        if (stCtx.ulSeq == channel_iter->second->GetSequence())
        {
            std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
            if (named_iter == m_mapNamedChannel.end())
            {
                std::list<Channel*> listChannel;
                listChannel.push_back(channel_iter->second);
                m_mapNamedChannel.insert(std::pair<std::string, std::list<Channel*> >(strIdentify, listChannel));
            }
            else
            {
                named_iter->second.push_back(channel_iter->second);
            }
            SetChannelIdentify(stCtx, strIdentify);
            return(true);
        }
        return(false);
    }
}

bool Worker::AddNamedChannel(const std::string& strIdentify, Channel* pChannel)
{
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
    if (named_iter == m_mapNamedChannel.end())
    {
        std::list<Channel*> listChannel;
        listChannel.push_back(pChannel);
        m_mapNamedChannel.insert(std::pair<std::string, std::list<Channel*> >(strIdentify, listChannel));
        return(true);
    }
    else
    {
        named_iter->second.push_back(pChannel);
    }
    pChannel->SetIdentify(strIdentify);
    return(true);
}

void Worker::DelNamedChannel(const std::string& strIdentify)
{
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
    if (named_iter == m_mapNamedChannel.end())
    {
        ;
    }
    else
    {
        m_mapNamedChannel.erase(named_iter);
    }
}

void Worker::AddNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator iter = m_mapIdentifyNodeType.find(strIdentify);
    if (iter == m_mapIdentifyNodeType.end())
    {
        m_mapIdentifyNodeType.insert(iter,
                std::pair<std::string, std::string>(strIdentify, strNodeType));

        T_MAP_NODE_TYPE_IDENTIFY::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(strNodeType);
        if (node_type_iter == m_mapNodeIdentify.end())
        {
            std::set<std::string> setIdentify;
            setIdentify.insert(strIdentify);
            std::pair<T_MAP_NODE_TYPE_IDENTIFY::iterator, bool> insert_node_result;
            insert_node_result = m_mapNodeIdentify.insert(std::pair< std::string,
                            std::pair<std::set<std::string>::iterator, std::set<std::string> > >(
                                            strNodeType, std::make_pair(setIdentify.begin(), setIdentify)));    //这里的setIdentify是临时变量，setIdentify.begin()将会成非法地址
            if (insert_node_result.second == false)
            {
                return;
            }
            insert_node_result.first->second.first = insert_node_result.first->second.second.begin();
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter == node_type_iter->second.second.end())
            {
                node_type_iter->second.second.insert(strIdentify);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
    }
}

void Worker::DelNodeIdentify(const std::string& strNodeType, const std::string& strIdentify)
{
    LOG4_TRACE("%s(%s, %s)", __FUNCTION__, strNodeType.c_str(), strIdentify.c_str());
    std::map<std::string, std::string>::iterator identify_iter = m_mapIdentifyNodeType.find(strIdentify);
    if (identify_iter != m_mapIdentifyNodeType.end())
    {
        std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
        node_type_iter = m_mapNodeIdentify.find(identify_iter->second);
        if (node_type_iter != m_mapNodeIdentify.end())
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.find(strIdentify);
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.second.erase(id_iter);
                node_type_iter->second.first = node_type_iter->second.second.begin();
            }
        }
        m_mapIdentifyNodeType.erase(identify_iter);
    }
}

bool Worker::Register(const std::string& strIdentify, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(strIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_DEBUG("redis context %s", strIdentify.c_str());
        return(Register(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_DEBUG("m_pLabor->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool Worker::Register(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    char szIdentify[32] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter != m_mapRedisContext.end())
    {
        LOG4_TRACE("redis context %s", szIdentify);
        return(Register(ctx_iter->second, pRedisStep));
    }
    else
    {
        LOG4_TRACE("m_pLabor->AutoRedisCmd(%s, %d)", strHost.c_str(), iPort);
        return(AutoRedisCmd(strHost, iPort, pRedisStep));
    }
}

bool Worker::AddRedisContextAddr(const std::string& strHost, int iPort, redisAsyncContext* ctx)
{
    LOG4_TRACE("%s(%s, %d, 0x%X)", __FUNCTION__, strHost.c_str(), iPort, ctx);
    char szIdentify[32] = {0};
    snprintf(szIdentify, 32, "%s:%d", strHost.c_str(), iPort);
    std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(szIdentify);
    if (ctx_iter == m_mapRedisContext.end())
    {
        m_mapRedisContext.insert(std::pair<std::string, const redisAsyncContext*>(szIdentify, ctx));
        std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
        if (identify_iter == m_mapContextIdentify.end())
        {
            m_mapContextIdentify.insert(std::pair<const redisAsyncContext*, std::string>(ctx, szIdentify));
        }
        else
        {
            identify_iter->second = szIdentify;
        }
        return(true);
    }
    else
    {
        return(false);
    }
}

void Worker::DelRedisContextAddr(const redisAsyncContext* ctx)
{
    std::map<const redisAsyncContext*, std::string>::iterator identify_iter = m_mapContextIdentify.find(ctx);
    if (identify_iter != m_mapContextIdentify.end())
    {
        std::map<std::string, const redisAsyncContext*>::iterator ctx_iter = m_mapRedisContext.find(identify_iter->second);
        if (ctx_iter != m_mapRedisContext.end())
        {
            m_mapRedisContext.erase(ctx_iter);
        }
        m_mapContextIdentify.erase(identify_iter);
    }
}

bool Worker::SendTo(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(fd %d, seq %lu) pWaitForSendBuff", __FUNCTION__, stCtx.iFd, stCtx.ulSeq);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            E_CODEC_STATUS eStatus = iter->second->Send();
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
    }
    return(false);
}

bool Worker::SendTo(const tagChannelContext& stCtx, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(fd %d, fd_seq %lu, cmd %u, msg_seq %u)",
                    __FUNCTION__, stCtx.iFd, stCtx.ulSeq, uiCmd, uiSeq);
    std::map<int, Channel*>::iterator conn_iter = m_mapChannel.find(stCtx.iFd);
    if (conn_iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->GetSequence() == stCtx.ulSeq)
        {
            E_CODEC_STATUS eStatus = conn_iter->second->Send(uiCmd, uiSeq, oMsgBody);
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.ulSeq, conn_iter->second->GetSequence());
            return(false);
        }
    }
}

bool Worker::SendTo(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
    if (named_iter == m_mapNamedChannel.end())
    {
        LOG4_TRACE("no channel match %s.", strIdentify.c_str());
        return(AutoSend(strIdentify, uiCmd, uiSeq, oMsgBody));
    }
    else
    {
        E_CODEC_STATUS eStatus = (*named_iter->second.begin())->Send(uiCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
        {
            return(true);
        }
        return(false);
    }
}

bool Worker::SendToNext(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.first != node_type_iter->second.second.end())
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.first;
            node_type_iter->second.first++;
            return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
        }
        else
        {
            std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
            if (id_iter != node_type_iter->second.second.end())
            {
                node_type_iter->second.first = id_iter;
                node_type_iter->second.first++;
                return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
            }
            else
            {
                LOG4_ERROR("no channel match and no node identify found for %s!", strNodeType.c_str());
                return(false);
            }
        }
    }
}

bool Worker::SendToWithMod(const std::string& strNodeType, unsigned int uiModFactor, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(nody_type: %s, mod_factor: %d)", __FUNCTION__, strNodeType.c_str(), uiModFactor);
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        if (node_type_iter->second.second.size() == 0)
        {
            LOG4_ERROR("no channel match %s!", strNodeType.c_str());
            return(false);
        }
        else
        {
            std::set<std::string>::iterator id_iter;
            int target_identify = uiModFactor % node_type_iter->second.second.size();
            int i = 0;
            for (i = 0, id_iter = node_type_iter->second.second.begin();
                            i < node_type_iter->second.second.size();
                            ++i, ++id_iter)
            {
                if (i == target_identify && id_iter != node_type_iter->second.second.end())
                {
                    return(SendTo(*id_iter, uiCmd, uiSeq, oMsgBody));
                }
            }
            return(false);
        }
    }
}

bool Worker::Broadcast(const std::string& strNodeType, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(node_type: %s)", __FUNCTION__, strNodeType.c_str());
    std::map<std::string, std::pair<std::set<std::string>::iterator, std::set<std::string> > >::iterator node_type_iter;
    node_type_iter = m_mapNodeIdentify.find(strNodeType);
    if (node_type_iter == m_mapNodeIdentify.end())
    {
        LOG4_ERROR("no channel match %s!", strNodeType.c_str());
        return(false);
    }
    else
    {
        int iSendNum = 0;
        for (std::set<std::string>::iterator id_iter = node_type_iter->second.second.begin();
                        id_iter != node_type_iter->second.second.end(); ++id_iter)
        {
            if (*id_iter != GetWorkerIdentify())
            {
                SendTo(*id_iter, uiCmd, uiSeq, oMsgBody);
            }
            ++iSendNum;
        }
        if (0 == iSendNum)
        {
            LOG4_ERROR("no channel match and no node identify found for %s!", strNodeType.c_str());
            return(false);
        }
    }
    return(true);
}

bool Worker::SendTo(const tagChannelContext& stCtx, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s(fd %d, seq %lu)", __FUNCTION__, stCtx.iFd, stCtx.ulSeq);
    std::map<int, Channel*>::iterator conn_iter = m_mapChannel.find(stCtx.iFd);
    if (conn_iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (conn_iter->second->GetSequence() == stCtx.ulSeq)
        {
            E_CODEC_STATUS eStatus = conn_iter->second->Send(oHttpMsg, uiHttpStepSeq);
            if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
            {
                return(true);
            }
            return(false);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.ulSeq, conn_iter->second->GetSequence());
            return(false);
        }
    }
}

bool Worker::SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    char szIdentify[256] = {0};
    snprintf(szIdentify, sizeof(szIdentify), "%s:%d%s", strHost.c_str(), iPort, strUrlPath.c_str());
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, szIdentify);
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(szIdentify);
    if (named_iter == m_mapNamedChannel.end())
    {
        LOG4_TRACE("no channel match %s.", szIdentify);
        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
    }
    else
    {
        for (std::list<Channel*>::iterator channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            if (0 == (*channel_iter)->GetStepSeq())
            {
                E_CODEC_STATUS eStatus = (*channel_iter)->Send(oHttpMsg, uiHttpStepSeq);
                if (CODEC_STATUS_OK == eStatus || CODEC_STATUS_PAUSE == eStatus)
                {
                    return(true);
                }
                return(false);
            }
        }
        return(AutoSend(strHost, iPort, strUrlPath, oHttpMsg, uiHttpStepSeq));
    }
}

bool Worker::SetChannelIdentify(const tagChannelContext& stCtx, const std::string& strIdentify)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        LOG4_ERROR("no fd %d found in m_mapChannel", stCtx.iFd);
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            iter->second->SetIdentify(strIdentify);
            return(true);
        }
        else
        {
            LOG4_ERROR("fd %d sequence %lu not match the sequence %lu in m_mapChannel",
                            stCtx.iFd, stCtx.ulSeq, iter->second->GetSequence());
            return(false);
        }
    }
}

bool Worker::AutoSend(const std::string& strIdentify, uint32 uiCmd, uint32 uiSeq, const MsgBody& oMsgBody)
{
    LOG4_TRACE("%s(%s)", __FUNCTION__, strIdentify.c_str());
    int iPosIpPortSeparator = strIdentify.find(':');
    if (iPosIpPortSeparator == std::string::npos)
    {
        return(false);
    }
    int iPosPortWorkerIndexSeparator = strIdentify.rfind('.');
    std::string strHost = strIdentify.substr(0, iPosIpPortSeparator);
    std::string strPort = strIdentify.substr(iPosIpPortSeparator + 1, iPosPortWorkerIndexSeparator - (iPosIpPortSeparator + 1));
    std::string strWorkerIndex = strIdentify.substr(iPosPortWorkerIndexSeparator + 1, std::string::npos);
    int iPort = atoi(strPort.c_str());
    if (iPort == 0)
    {
        return(false);
    }
    int iWorkerIndex = atoi(strWorkerIndex.c_str());
    if (iWorkerIndex > 200)
    {
        return(false);
    }
    struct sockaddr_in stAddr;
    int iFd = -1;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    Channel* pChannel = CreateChannel(iFd, CODEC_PROTOBUF);
    if (NULL != pChannel)
    {
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        pChannel->SetIdentify(strIdentify);
        pChannel->SetRemoteAddr(strHost);
        pChannel->SetChannelStatus(CHANNEL_STATUS_INIT);
        E_CODEC_STATUS eCodecStatus = pChannel->Send(uiCmd, uiSeq, oMsgBody);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardChannel(pChannel);
        }

        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        pChannel->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT);
        m_mapSeq2WorkerIndex.insert(std::pair<uint32, int>(pChannel->GetSequence(), iWorkerIndex));
        AddNamedChannel(strIdentify, pChannel);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool Worker::AutoSend(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg, uint32 uiHttpStepSeq)
{
    LOG4_TRACE("%s(%s, %d, %s)", __FUNCTION__, strHost.c_str(), iPort, strUrlPath.c_str());
    struct sockaddr_in stAddr;
    int iFd;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iPort);
    stAddr.sin_addr.s_addr = inet_addr(strHost.c_str());
    if (stAddr.sin_addr.s_addr == 4294967295 || stAddr.sin_addr.s_addr == 0)
    {
        struct hostent *he;
        he = gethostbyname(strHost.c_str());
        if (he != NULL)
        {
            stAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(he->h_addr)));
        }
        else
        {
            LOG4_ERROR("gethostbyname(%s) error!", strHost.c_str());
            return(false);
        }
    }
    bzero(&(stAddr.sin_zero), 8);
    iFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iFd == -1)
    {
        return(false);
    }
    x_sock_set_block(iFd, 0);
    int nREUSEADDR = 1;
    setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int));
    Channel* pChannel = CreateChannel(iFd, CODEC_HTTP);
    if (NULL != pChannel)
    {
        char szIdentify[32] = {0};
        AddIoTimeout(pChannel, 1.5);
        AddIoReadEvent(pChannel);
        AddIoWriteEvent(pChannel);
        snprintf(szIdentify, sizeof(szIdentify), "%s:%d", strHost.c_str(), iPort);
        pChannel->SetIdentify(szIdentify);
        pChannel->SetRemoteAddr(strHost);
        pChannel->SetChannelStatus(CHANNEL_STATUS_INIT);
        E_CODEC_STATUS eCodecStatus = pChannel->Send(oHttpMsg, uiHttpStepSeq);
        if (CODEC_STATUS_OK != eCodecStatus && CODEC_STATUS_PAUSE != eCodecStatus)
        {
            DiscardChannel(pChannel);
        }

        connect(iFd, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
        pChannel->SetChannelStatus(CHANNEL_STATUS_TRY_CONNECT, uiHttpStepSeq);
        AddNamedChannel(szIdentify, pChannel);
        return(true);
    }
    else    // 没有足够资源分配给新连接，直接close掉
    {
        close(iFd);
        return(false);
    }
}

bool Worker::AutoRedisCmd(const std::string& strHost, int iPort, RedisStep* pRedisStep)
{
    LOG4_TRACE("%s() redisAsyncConnect(%s, %d)", __FUNCTION__, strHost.c_str(), iPort);
    redisAsyncContext *c = redisAsyncConnect(strHost.c_str(), iPort);
    if (c->err)
    {
        /* Let *c leak for now... */
        LOG4_ERROR("error: %s", c->errstr);
        return(false);
    }
    c->data = this;
    tagRedisAttr* pRedisAttr = NULL;
    try
    {
        pRedisAttr = new tagRedisAttr();
    }
    catch(std::bad_alloc& e)
    {
        LOG4_ERROR("new tagRedisAttr error: %s", e.what());
        return(false);
    }
    pRedisAttr->ulSeq = GetSequence();
    pRedisAttr->listWaitData.push_back(pRedisStep);
    pRedisStep->SetLogger(&m_oLogger);
    pRedisStep->SetLabor(this);
    pRedisStep->SetRegistered();
    m_mapRedisAttr.insert(std::pair<redisAsyncContext*, tagRedisAttr*>(c, pRedisAttr));
    redisLibevAttach(m_loop, c);
    redisAsyncSetConnectCallback(c, RedisConnectCallback);
    redisAsyncSetDisconnectCallback(c, RedisDisconnectCallback);
    AddRedisContextAddr(strHost, iPort, c);
    return(true);
}

void Worker::AddInnerChannel(const tagChannelContext& stCtx)
{
    std::map<int, uint32>::iterator iter = m_mapInnerFd.find(stCtx.iFd);
    if (iter == m_mapInnerFd.end())
    {
        m_mapInnerFd.insert(std::pair<int, uint32>(stCtx.iFd, stCtx.ulSeq));
    }
    else
    {
        iter->second = stCtx.ulSeq;
    }
    LOG4_TRACE("%s() now m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
}

bool Worker::SetClientData(const tagChannelContext& stCtx, const std::string& strClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            iter->second->SetClientData(strClientData);
            return(true);
        }
        else
        {
            return(false);
        }
    }
}

std::string Worker::GetClientAddr(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        return("");
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            return(iter->second->GetRemoteAddr());
        }
        else
        {
            return("");
        }
    }
}

bool Worker::DiscardNamedChannel(const std::string& strIdentify)
{
    LOG4_TRACE("%s(identify: %s)", __FUNCTION__, strIdentify.c_str());
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(strIdentify);
    if (named_iter == m_mapNamedChannel.end())
    {
        LOG4_DEBUG("no channel match %s.", strIdentify.c_str());
        return(false);
    }
    else
    {
        for (std::list<Channel*>::iterator channel_iter = named_iter->second.begin();
                channel_iter != named_iter->second.end(); ++channel_iter)
        {
            (*channel_iter)->SetIdentify("");
            (*channel_iter)->SetClientData("");
        }
        named_iter->second.clear();
        m_mapNamedChannel.erase(named_iter);
        return(true);
    }
}

bool Worker::SwitchCodec(const tagChannelContext& stCtx, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s()", __FUNCTION__);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter == m_mapChannel.end())
    {
        return(false);
    }
    else
    {
        if (iter->second->GetSequence() == stCtx.ulSeq)
        {
            return(iter->second->SwitchCodec(eCodecType, m_dIoTimeout));
        }
        else
        {
            return(false);
        }
    }
}

void Worker::ExecStep(uint32 uiCallerStepSeq, uint32 uiCalledStepSeq,
                int iErrno, const std::string& strErrMsg, const std::string& strErrShow)
{
    LOG4_TRACE("%s(caller[%u], called[%u])", __FUNCTION__, uiCallerStepSeq, uiCalledStepSeq);
    std::map<uint32, Step*>::iterator step_iter = m_mapCallbackStep.find(uiCalledStepSeq);
    if (step_iter == m_mapCallbackStep.end())
    {
        LOG4_WARN("step %u is not in the callback list.", uiCalledStepSeq);
    }
    else
    {
        if (CMD_STATUS_RUNNING != step_iter->second->Emit(iErrno, strErrMsg))
        {
            Remove(uiCallerStepSeq, step_iter->second);
        }
    }
}

void Worker::LoadCmd(CJsonObject& oCmdConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    int iCmd = 0;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<int, tagSo*>::iterator cmd_iter;
    tagSo* pSo = NULL;
    for (int i = 0; i < oCmdConf.GetArraySize(); ++i)
    {
        oCmdConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oCmdConf[i].Get("cmd", iCmd) && oCmdConf[i].Get("version", iVersion))
            {
                cmd_iter = m_mapCmd.find(iCmd);
                if (cmd_iter == m_mapCmd.end())
                {
                    strSoPath = m_strWorkPath + std::string("/") + oCmdConf[i]("so_path");
                    pSo = LoadSoAndGetCmd(iCmd, strSoPath, oCmdConf[i]("entrance_symbol"), iVersion);
                    if (pSo != NULL)
                    {
                        LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                        m_mapCmd.insert(std::pair<int, tagSo*>(iCmd, pSo));
                    }
                }
                else
                {
                    if (iVersion != cmd_iter->second->iVersion)
                    {
                        strSoPath = m_strWorkPath + std::string("/") + oCmdConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        pSo = LoadSoAndGetCmd(iCmd, strSoPath, oCmdConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pSo != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            delete cmd_iter->second;
                            cmd_iter->second = pSo;
                        }
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oCmdConf[i].Get("cmd", iCmd))
            {
                strSoPath = m_strWorkPath + std::string("/") + oCmdConf[i]("so_path");
                UnloadSoAndDeleteCmd(iCmd);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
    }
}

tagSo* Worker::LoadSoAndGetCmd(int iCmd, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagSo* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        if (pHandle != NULL)
        {
            dlclose(pHandle);
        }
        return(pSo);
    }
    CreateCmd* pCreateCmd = (CreateCmd*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Cmd* pCmd = pCreateCmd();
    if (pCmd != NULL)
    {
        pSo = new tagSo();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pCmd = pCmd;
            pSo->iVersion = iVersion;
            pCmd->SetLogger(&m_oLogger);
            pCmd->SetLabor(this);
            pCmd->SetCmd(iCmd);
            if (!pCmd->Init())
            {
                LOG4_FATAL("Cmd %d %s init error",
                                iCmd, strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pCmd;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void Worker::UnloadSoAndDeleteCmd(int iCmd)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapCmd.find(iCmd);
    if (cmd_iter != m_mapCmd.end())
    {
        delete cmd_iter->second;
        cmd_iter->second = NULL;
        m_mapCmd.erase(cmd_iter);
    }
}

void Worker::LoadModule(CJsonObject& oModuleConf)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::string strModulePath;
    int iVersion = 0;
    bool bIsload = false;
    std::string strSoPath;
    std::map<std::string, tagModule*>::iterator module_iter;
    tagModule* pModule = NULL;
    LOG4_TRACE("oModuleConf.GetArraySize() = %d", oModuleConf.GetArraySize());
    for (int i = 0; i < oModuleConf.GetArraySize(); ++i)
    {
        oModuleConf[i].Get("load", bIsload);
        if (bIsload)
        {
            if (oModuleConf[i].Get("url_path", strModulePath) && oModuleConf[i].Get("version", iVersion))
            {
                LOG4_TRACE("url_path = %s", strModulePath.c_str());
                module_iter = m_mapModule.find(strModulePath);
                if (module_iter == m_mapModule.end())
                {
                    strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                    if (0 != access(strSoPath.c_str(), F_OK))
                    {
                        LOG4_WARN("%s not exist!", strSoPath.c_str());
                        continue;
                    }
                    pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                    if (pModule != NULL)
                    {
                        LOG4_INFO("succeed in loading %s with module path \"%s\".",
                                        strSoPath.c_str(), strModulePath.c_str());
                        m_mapModule.insert(std::pair<std::string, tagModule*>(strModulePath, pModule));
                    }
                }
                else
                {
                    if (iVersion != module_iter->second->iVersion)
                    {
                        strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                        if (0 != access(strSoPath.c_str(), F_OK))
                        {
                            LOG4_WARN("%s not exist!", strSoPath.c_str());
                            continue;
                        }
                        pModule = LoadSoAndGetModule(strModulePath, strSoPath, oModuleConf[i]("entrance_symbol"), iVersion);
                        LOG4_TRACE("%s:%d after LoadSoAndGetCmd", __FILE__, __LINE__);
                        if (pModule != NULL)
                        {
                            LOG4_INFO("succeed in loading %s", strSoPath.c_str());
                            delete module_iter->second;
                            module_iter->second = pModule;
                        }
                    }
                }
            }
        }
        else        // 卸载动态库
        {
            if (oModuleConf[i].Get("url_path", strModulePath))
            {
                strSoPath = m_strWorkPath + std::string("/") + oModuleConf[i]("so_path");
                UnloadSoAndDeleteModule(strModulePath);
                LOG4_INFO("unload %s", strSoPath.c_str());
            }
        }
    }
}

tagModule* Worker::LoadSoAndGetModule(const std::string& strModulePath, const std::string& strSoPath, const std::string& strSymbol, int iVersion)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    tagModule* pSo = NULL;
    void* pHandle = NULL;
    pHandle = dlopen(strSoPath.c_str(), RTLD_NOW);
    char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("cannot load dynamic lib %s!" , dlsym_error);
        return(pSo);
    }
    CreateModule* pCreateModule = (CreateModule*)dlsym(pHandle, strSymbol.c_str());
    dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOG4_FATAL("dlsym error %s!" , dlsym_error);
        dlclose(pHandle);
        return(pSo);
    }
    Module* pModule = (Module*)pCreateModule();
    if (pModule != NULL)
    {
        pSo = new tagModule();
        if (pSo != NULL)
        {
            pSo->pSoHandle = pHandle;
            pSo->pModule = pModule;
            pSo->iVersion = iVersion;
            pModule->SetLogger(&m_oLogger);
            pModule->SetLabor(this);
            pModule->SetModulePath(strModulePath);
            if (!pModule->Init())
            {
                LOG4_FATAL("Module %s %s init error", strModulePath.c_str(), strSoPath.c_str());
                delete pSo;
                pSo = NULL;
            }
        }
        else
        {
            LOG4_FATAL("new tagSo() error!");
            delete pModule;
            dlclose(pHandle);
        }
    }
    return(pSo);
}

void Worker::UnloadSoAndDeleteModule(const std::string& strModulePath)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<std::string, tagModule*>::iterator module_iter;
    module_iter = m_mapModule.find(strModulePath);
    if (module_iter != m_mapModule.end())
    {
        delete module_iter->second;
        module_iter->second = NULL;
        m_mapModule.erase(module_iter);
    }
}

bool Worker::AddPeriodicTaskEvent()
{
    LOG4_TRACE("%s()", __FUNCTION__);
    ev_timer* timeout_watcher = (ev_timer*)malloc(sizeof(ev_timer));
    if (timeout_watcher == NULL)
    {
        LOG4_ERROR("malloc timeout_watcher error!");
        return(false);
    }
    ev_timer_init (timeout_watcher, PeriodicTaskCallback, NODE_BEAT + ev_time() - ev_now(m_loop), 0.);
    timeout_watcher->data = (void*)this;
    ev_timer_start (m_loop, timeout_watcher);
    return(true);
}

bool Worker::AddIoReadEvent(Channel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_READ);
        ev_io_start (m_loop, io_watcher);
        return(true);
    }
    else
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_READ);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool Worker::AddIoWriteEvent(Channel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        io_watcher = pChannel->AddIoWatcher();
        if (NULL == io_watcher)
        {
            return(false);
        }
        ev_io_init (io_watcher, IoCallback, pChannel->GetFd(), EV_WRITE);
        ev_io_start (m_loop, io_watcher);
        return(true);
    }
    else
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events | EV_WRITE);
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool Worker::RemoveIoWriteEvent(Channel* pChannel)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_io* io_watcher = pChannel->MutableIoWatcher();
    if (NULL == io_watcher)
    {
        return(false);
    }
    if (EV_WRITE & io_watcher->events)
    {
        ev_io_stop(m_loop, io_watcher);
        ev_io_set(io_watcher, io_watcher->fd, io_watcher->events & (~EV_WRITE));
        ev_io_start (m_loop, io_watcher);
    }
    return(true);
}

bool Worker::AddIoTimeout(Channel* pChannel, ev_tstamp dTimeout)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, pChannel->GetFd(), pChannel->GetSequence());
    ev_timer* timer_watcher = pChannel->MutableTimerWatcher();
    if (NULL == timer_watcher)
    {
        timer_watcher = pChannel->AddTimerWatcher();
        LOG4_TRACE("pChannel = 0x%d,  timer_watcher = 0x%d", pChannel, timer_watcher);
        if (NULL == timer_watcher)
        {
            return(false);
        }
        // @deprecated pChannel->SetKeepAlive(m_dIoTimeout);
        ev_timer_init (timer_watcher, IoTimeoutCallback, dTimeout + ev_time() - ev_now(m_loop), 0.);
        ev_timer_start (m_loop, timer_watcher);
        return(true);
    }
    else
    {
        LOG4_TRACE("pChannel = 0x%d,  timer_watcher = 0x%d", pChannel, timer_watcher);
        ev_timer_stop(m_loop, timer_watcher);
        ev_timer_set(timer_watcher, dTimeout + ev_time() - ev_now(m_loop), 0);
        ev_timer_start (m_loop, timer_watcher);
        return(true);
    }
}

bool Worker::AddIoTimeout(const tagChannelContext& stCtx)
{
    LOG4_TRACE("%s(%d, %u)", __FUNCTION__, stCtx.iFd, stCtx.ulSeq);
    std::map<int, Channel*>::iterator iter = m_mapChannel.find(stCtx.iFd);
    if (iter != m_mapChannel.end())
    {
        if (stCtx.ulSeq == iter->second->GetSequence())
        {
            ev_timer* timer_watcher = iter->second->MutableTimerWatcher();
            if (NULL == timer_watcher)
            {
                timer_watcher = iter->second->AddTimerWatcher();
                LOG4_TRACE("timer_watcher = 0x%d", timer_watcher);
                if (NULL == timer_watcher)
                {
                    return(false);
                }
                ev_timer_init (timer_watcher, IoTimeoutCallback, iter->second->GetKeepAlive() + ev_time() - ev_now(m_loop), 0.);
                ev_timer_start (m_loop, timer_watcher);
                return(true);
            }
            else
            {
                LOG4_TRACE("timer_watcher = 0x%d", timer_watcher);
                ev_timer_stop(m_loop, timer_watcher);
                ev_timer_set(timer_watcher, iter->second->GetKeepAlive() + ev_time() - ev_now(m_loop), 0);
                ev_timer_start (m_loop, timer_watcher);
                return(true);
            }
        }
    }
    return(false);
}

Channel* Worker::CreateChannel(int iFd, E_CODEC_TYPE eCodecType)
{
    LOG4_DEBUG("%s(iFd %d, codec_type %d)", __FUNCTION__, iFd, eCodecType);
    std::map<int, Channel*>::iterator iter;
    iter = m_mapChannel.find(iFd);
    if (iter == m_mapChannel.end())
    {
        Channel* pChannel = NULL;
        try
        {
            pChannel = new Channel(iFd, GetSequence());
        }
        catch(std::bad_alloc& e)
        {
            LOG4_ERROR("new channel for fd %d error: %s", e.what());
            return(NULL);
        }
        pChannel->SetLabor(this);
        pChannel->SetLogger(&m_oLogger);
        if (pChannel->Init(eCodecType))
        {
            m_mapChannel.insert(std::pair<int, Channel*>(iFd, pChannel));
            return(pChannel);
        }
        else
        {
            DELETE(pChannel);
            return(NULL);
        }
    }
    else
    {
        LOG4_WARN("fd %d is exist!", iFd);
        return(iter->second);
    }
}

bool Worker::DiscardChannel(Channel* pChannel, bool bChannelNotice)
{
    LOG4_DEBUG("%s disconnect, identify %s", pChannel->GetRemoteAddr().c_str(), pChannel->GetIdentify().c_str());
    if (CHANNEL_STATUS_DISCARD == pChannel->GetChannelStatus() || CHANNEL_STATUS_DESTROY == pChannel->GetChannelStatus())
    {
        return(false);
    }
    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(pChannel->GetFd());
    if (inner_iter != m_mapInnerFd.end())
    {
        LOG4_TRACE("%s() m_mapInnerFd.size() = %u", __FUNCTION__, m_mapInnerFd.size());
        m_mapInnerFd.erase(inner_iter);
    }
    std::map<std::string, std::list<Channel*> >::iterator named_iter = m_mapNamedChannel.find(pChannel->GetIdentify());
    if (named_iter != m_mapNamedChannel.end())
    {
        for (std::list<Channel*>::iterator it = named_iter->second.begin();
                it != named_iter->second.end(); ++it)
        {
            if ((*it)->GetSequence() == pChannel->GetFd())
            {
                named_iter->second.erase(it);
                break;
            }
        }
        if (0 == named_iter->second.size())
        {
            m_mapNamedChannel.erase(named_iter);
        }
    }
    if (bChannelNotice)
    {
        tagChannelContext stCtx;
        stCtx.iFd = pChannel->GetFd();
        stCtx.ulSeq = pChannel->GetSequence();
        ChannelNotice(stCtx, pChannel->GetIdentify(), pChannel->GetClientData());
    }
    ev_io_stop (m_loop, pChannel->MutableIoWatcher());
    if (NULL != pChannel->MutableTimerWatcher())
    {
        ev_timer_stop (m_loop, pChannel->MutableTimerWatcher());
    }
    std::map<int, Channel*>::iterator channel_iter = m_mapChannel.find(pChannel->GetFd());
    if (channel_iter != m_mapChannel.end() && channel_iter->second->GetSequence() == pChannel->GetSequence())
    {
        m_mapChannel.erase(channel_iter);
    }
    pChannel->SetChannelStatus(CHANNEL_STATUS_DISCARD);
    return(true);
}

void Worker::ChannelNotice(const tagChannelContext& stCtx, const std::string& strIdentify, const std::string& strClientData)
{
    LOG4_TRACE("%s()", __FUNCTION__);
    std::map<int32, tagSo*>::iterator cmd_iter;
    cmd_iter = m_mapCmd.find(CMD_REQ_DISCONNECT);
    if (cmd_iter != m_mapCmd.end() && cmd_iter->second != NULL)
    {
        MsgHead oMsgHead;
        MsgBody oMsgBody;
        oMsgBody.mutable_req_target()->set_route_id(0);
        oMsgBody.set_data(strIdentify);
        oMsgBody.set_add_on(strClientData);
        oMsgHead.set_cmd(CMD_REQ_DISCONNECT);
        oMsgHead.set_seq(GetSequence());
        oMsgHead.set_len(oMsgBody.ByteSize());
        ((Cmd*)cmd_iter->second->pCmd)->AnyMessage(stCtx, oMsgHead, oMsgBody);
    }
}

bool Worker::Handle(Channel* pChannel, const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    LOG4_DEBUG("%s(cmd %u, seq %lu)", __FUNCTION__, oMsgHead.cmd(), oMsgHead.seq());
    tagChannelContext stCtx;
    stCtx.iFd = pChannel->GetFd();
    stCtx.ulSeq = pChannel->GetSequence();
    if (gc_uiCmdReq & oMsgHead.cmd())    // 新请求
    {
        MsgHead oOutMsgHead;
        MsgBody oOutMsgBody;
        std::map<int32, Cmd*>::iterator cmd_iter;
        cmd_iter = m_mapPreloadCmd.find(gc_uiCmdBit & oMsgHead.cmd());
        if (cmd_iter != m_mapPreloadCmd.end() && cmd_iter->second != NULL)
        {
            cmd_iter->second->AnyMessage(stCtx, oMsgHead, oMsgBody);
        }
        else
        {
            std::map<int, tagSo*>::iterator cmd_so_iter;
            cmd_so_iter = m_mapCmd.find(gc_uiCmdBit & oMsgHead.cmd());
            if (cmd_so_iter != m_mapCmd.end() && cmd_so_iter->second != NULL)
            {
                ((Cmd*)cmd_so_iter->second->pCmd)->AnyMessage(stCtx, oMsgHead, oMsgBody);
            }
            else        // 没有对应的cmd，是需由接入层转发的请求
            {
                if (CMD_REQ_SET_LOG_LEVEL == oMsgHead.cmd())
                {
                    LogLevel oLogLevel;
                    oLogLevel.ParseFromString(oMsgBody.data());
                    LOG4_INFO("log level set to %d", oLogLevel.log_level());
                    m_oLogger.setLogLevel(oLogLevel.log_level());
                }
                else if (CMD_REQ_RELOAD_SO == oMsgHead.cmd())
                {
                    CJsonObject oSoConfJson;
                    LoadCmd(oSoConfJson);
                }
                else if (CMD_REQ_RELOAD_MODULE == oMsgHead.cmd())
                {
                    CJsonObject oModuleConfJson;
                    LoadModule(oModuleConfJson);
                }
                else
                {
#ifdef NODE_TYPE_ACCESS
                    std::map<int, uint32>::iterator inner_iter = m_mapInnerFd.find(stCtx.iFd);
                    if (inner_iter != m_mapInnerFd.end())   // 内部服务往客户端发送  if (std::string("0.0.0.0") == strFromIp)
                    {
                        cmd_so_iter = m_mapCmd.find(CMD_REQ_TO_CLIENT);
                        if (cmd_so_iter != m_mapCmd.end())
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stCtx, oMsgHead, oMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                            oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oMsgHead.seq());
                            oOutMsgHead.set_len(oOutMsgBody.ByteSize());
                        }
                    }
                    else
                    {
                        cmd_so_iter = m_mapCmd.find(CMD_REQ_FROM_CLIENT);
                        if (cmd_so_iter != m_mapCmd.end() && cmd_so_iter->second != NULL)
                        {
                            cmd_so_iter->second->pCmd->AnyMessage(stCtx, oMsgHead, oMsgBody);
                        }
                        else
                        {
                            snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                            LOG4_ERROR(m_pErrBuff);
                            oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                            oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                            oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                            oOutMsgHead.set_seq(oMsgHead.seq());
                            oOutMsgHead.set_len(oOutMsgBody.ByteSize());
                        }
                    }
#else
                    snprintf(m_pErrBuff, gc_iErrBuffLen, "no handler to dispose cmd %u!", oMsgHead.cmd());
                    LOG4_ERROR(m_pErrBuff);
                    oOutMsgBody.mutable_rsp_result()->set_code(ERR_UNKNOWN_CMD);
                    oOutMsgBody.mutable_rsp_result()->set_msg(m_pErrBuff);
                    oOutMsgHead.set_cmd(CMD_RSP_SYS_ERROR);
                    oOutMsgHead.set_seq(oMsgHead.seq());
                    oOutMsgHead.set_len(oOutMsgBody.ByteSize());
#endif
                }
            }
        }
        if (CMD_RSP_SYS_ERROR == oOutMsgHead.cmd())
        {
            LOG4_TRACE("no cmd handler.");
            SendTo(stCtx, oOutMsgHead.cmd(), oOutMsgHead.seq(), oOutMsgBody);
            return(false);
        }
    }
    else    // 回调
    {
        std::map<uint32, Step*>::iterator step_iter;
        step_iter = m_mapCallbackStep.find(oMsgHead.seq());
        if (step_iter != m_mapCallbackStep.end())   // 步骤回调
        {
            LOG4_TRACE("receive message, cmd = %d",
                            oMsgHead.cmd());
            if (step_iter->second != NULL)
            {
                E_CMD_STATUS eResult;
                step_iter->second->SetActiveTime(ev_now(m_loop));
                LOG4_TRACE("cmd %u, seq %u, step_seq %u, step addr 0x%x, active_time %lf",
                                oMsgHead.cmd(), oMsgHead.seq(), step_iter->second->GetSequence(),
                                step_iter->second, step_iter->second->GetActiveTime());
                eResult = ((PbStep*)step_iter->second)->Callback(stCtx, oMsgHead, oMsgBody);
                if (CMD_STATUS_RUNNING != eResult)
                {
                    Remove(step_iter->second);
                }
            }
        }
        else
        {
            snprintf(m_pErrBuff, gc_iErrBuffLen, "no callback or the callback for seq %lu had been timeout!", oMsgHead.seq());
            LOG4_WARN(m_pErrBuff);
            return(false);
        }
    }
    return(true);
}

bool Worker::Handle(Channel* pChannel, const HttpMsg& oHttpMsg)
{
    LOG4_DEBUG("%s() oInHttpMsg.type() = %d, oInHttpMsg.path() = %s",
                    __FUNCTION__, oHttpMsg.type(), oHttpMsg.path().c_str());
    tagChannelContext stCtx;
    stCtx.iFd = pChannel->GetFd();
    stCtx.ulSeq = pChannel->GetSequence();
    if (HTTP_REQUEST == oHttpMsg.type())    // 新请求
    {
        std::map<std::string, tagModule*>::iterator module_iter;
        module_iter = m_mapModule.find(oHttpMsg.path());
        if (module_iter == m_mapModule.end())
        {
            module_iter = m_mapModule.find("/switch");
            if (module_iter == m_mapModule.end())
            {
                HttpMsg oOutHttpMsg;
                snprintf(m_pErrBuff, gc_iErrBuffLen, "no module to dispose %s!", oHttpMsg.path().c_str());
                LOG4_ERROR(m_pErrBuff);
                oOutHttpMsg.set_type(HTTP_RESPONSE);
                oOutHttpMsg.set_status_code(404);
                oOutHttpMsg.set_http_major(oHttpMsg.http_major());
                oOutHttpMsg.set_http_minor(oHttpMsg.http_minor());
                pChannel->Send(oOutHttpMsg, 0);
            }
            else
            {
                ((Module*)module_iter->second->pModule)->AnyMessage(stCtx, oHttpMsg);
            }
        }
        else
        {
            ((Module*)module_iter->second->pModule)->AnyMessage(stCtx, oHttpMsg);
        }
    }
    else
    {
        std::map<uint32, Step*>::iterator http_step_iter = m_mapCallbackStep.find(pChannel->GetStepSeq());
        if (http_step_iter == m_mapCallbackStep.end())
        {
            LOG4_ERROR("no callback for http response from %s!", oHttpMsg.url().c_str());
            pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
        }
        else
        {
            E_CMD_STATUS eResult;
            http_step_iter->second->SetActiveTime(ev_now(m_loop));
            eResult = ((HttpStep*)http_step_iter->second)->Callback(stCtx, oHttpMsg);
            if (CMD_STATUS_RUNNING != eResult)
            {
                Remove(http_step_iter->second);
                pChannel->SetChannelStatus(CHANNEL_STATUS_ESTABLISHED, 0);
            }
        }
    }
    return(true);
}

} /* namespace neb */
