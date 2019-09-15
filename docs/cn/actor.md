&emsp;&emsp;Actor类自身提供各组件与框架交互的成员函数，通过这些成员函数即可完成所有业务逻辑所需的功能，换句话说，基于Nebula框架的业务开发只需了解Actor类的30余成员函数的功能就足够。这样设计一方面是为了尽可能地降低Nebula学习和使用门槛，另一方面又能非常巧妙地带来一些令人惊喜的有且而强大的功能。

&emsp;&emsp;Actor类是所有Actor组件的基类，但Actor类不是一个虚基类，它更像是一个工具类。工具类通常会以组合的方式嵌入到其他类成员中使用，而Actor以继承的方式是因为Nebula的设计是所有组件所有逻辑皆Actor，是is关系不是has关系继承，更重要的是继承方式在Nebula可以很方便地实现一些组合方式难以实现的强大功能。

&emsp;&emsp;直接从Actor类派生的类都是ACT_UNDEFINE类型，Nebula框架不会管理这些类，所以请勿直接从Actor类派生，除非有非常充分且合理的理由直接从Actor类派生业务自己的Actor基类并自己管理。

### Actor类定义

&emsp;&emsp;Actor类继承std::enable_shared_from_this<Actor>是为了方便各Actor派生类方便从this指针获取shared_ptr进行共享，开发者无需关注std::enable_shared_from_this。

&emsp;&emsp;Actor类的ACTOR_TYPE枚举定义了Nebula框架支持的所有Actor组件类型，Cmd、Module、Step、Session等组件会自动填充ACTOR_TYPE，从这些组件派生的业务类无需关注ACTOR_TYPE，ACTOR_TYPE主要是Nebula框架管理组件用的。

&emsp;&emsp;Logger()模板函数是写日志用的，但并不需要直接调用Logger()函数写日志，调用LOG4_INFO()/LOG4_ERROR()等相关宏写日志即可，实际上这些宏就是调用Logger()函数写的日志，宏会自动填充一些文件名、函数名之类参数，比直接调用Logger()函数简单。Logger()函数的实现更是无须关注，提示一下，Nebula提供不需要开发者做任何额外工作的日志跟踪服务，其原理就在Logger()函数的实现里，这也是令人惊喜的有用且强大的功能之一。

&emsp;&emsp;MakeSharedStep()/MakeSharedSession()等相关模板函数用于通过类名反射动态创建具体业务逻辑类，动态创建具体Actor实例的同时会初始化并向框架进行注册，还有一些上下文信息的传递也会在动态创建的过程中完成传递。GetActorName()、GetTraceId()、GetSequence()、GetContext()等用起来觉得很方便很神奇，这些都是MakeShared相关模板函数默默地做了许多复杂的事情，把简单都留给用户。所以，切记不要自己去调用new来创建Actor的各种派生类，都调用MakeSharedStep/MakeSharedSession()等相关模板函数。至于非Actor组件派生类和用户自己的数据结构，MakeShared相关模板函数是不支持的，也没必要支持。

```C++
class Actor: public std::enable_shared_from_this<Actor>
{
public:
    enum ACTOR_TYPE
    {
        ACT_UNDEFINE            = 0,        ///< 未定义
        ACT_CMD                 = 1,        ///< Cmd对象，处理带命令字的pb请求
        ACT_MODULE              = 2,        ///< Module对象，处理带url path的http请求
        ACT_SESSION             = 3,        ///< Session会话对象
        ACT_TIMER               = 4,        ///< 定时器对象
        ACT_CONTEXT             = 5,        ///< 会话上下文对象
        ACT_PB_STEP             = 6,        ///< Step步骤对象，处理pb请求或响应
        ACT_HTTP_STEP           = 7,        ///< Step步骤对象，处理http请求或响应
        ACT_REDIS_STEP          = 8,        ///< Step步骤对象，处理redis请求或响应
        ACT_MODEL               = 9,        ///< Model模型对象，Model（IO无关）与Step（异步IO相关）共同构成功能链
        ACT_CHAIN               = 10,       ///< Chain链对象，用于将Model和Step组合成功能链
    };

public:
    Actor(ACTOR_TYPE eActorType = ACT_UNDEFINE, ev_tstamp dTimeout = 0.0);
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    virtual ~Actor();

    template <typename ...Targs> void Logger(int iLogLevel, const char* szFileName, unsigned int uiFileLine, const char* szFunction, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Step> MakeSharedStep(const std::string& strStepName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Session> MakeSharedSession(const std::string& strSessionName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Context> MakeSharedContext(const std::string& strContextName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Chain> MakeSharedChain(const std::string& strChainName, Targs&&... args);
    template <typename ...Targs> std::shared_ptr<Actor> MakeSharedActor(const std::string& strActorName, Targs&&... args);

    ACTOR_TYPE GetActorType() const
    {
        return(m_eActorType);
    }

    const std::string& GetActorName() const
    {
        return(m_strActorName);
    }

    const std::string& GetTraceId() const
    {
        return(m_strTraceId);
    }

    uint32 GetSequence();

protected:
    uint32 GetNodeId() const;
    uint32 GetWorkerIndex() const;
    ev_tstamp GetDefaultTimeout() const;
    const std::string& GetNodeType() const;
    const std::string& GetWorkPath() const;
    const std::string& GetNodeIdentify() const;
    time_t GetNowTime() const;
    const CJsonObject& GetCustomConf() const;

    std::shared_ptr<Session> GetSession(uint32 uiSessionId);
    std::shared_ptr<Session> GetSession(const std::string& strSessionId);
    bool ExecStep(uint32 uiStepSeq, int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);
    std::shared_ptr<Model> GetModel(const std::string& strModelName);
    std::shared_ptr<Context> GetContext();
    void SetContext(std::shared_ptr<Context> pContext);
    void AddAssemblyLine(std::shared_ptr<Session> pSession);

protected:
    bool SendTo(std::shared_ptr<SocketChannel> pChannel);
    bool SendTo(std::shared_ptr<SocketChannel> pChannel, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendTo(std::shared_ptr<SocketChannel> pChannel, const HttpMsg& oHttpMsg);
    bool SendTo(const std::string& strIdentify, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendTo(const std::string& strHost, int iPort, const std::string& strUrlPath, const HttpMsg& oHttpMsg);
    bool SendTo(std::shared_ptr<RedisChannel> pChannel);
    bool SendTo(const std::string& strIdentify);
    bool SendTo(const std::string& strHost, int iPort);
    bool SendRoundRobin(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendOriented(const std::string& strNodeType, uint32 uiFactor, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);
    bool SendOriented(const std::string& strNodeType, int32 iCmd, uint32 uiSeq, const MsgBody& oMsgBody);

protected:
    virtual void SetActiveTime(ev_tstamp dActiveTime)
    {
        m_dActiveTime = dActiveTime;
    }

    ev_tstamp GetActiveTime() const
    {
        return(m_dActiveTime);
    }

    ev_tstamp GetTimeout() const
    {
        return(m_dTimeout);
    }

private:
    void SetWorker(Worker* pWorker);
    ev_timer* MutableTimerWatcher();
    void SetActorName(const std::string& strActorName);
    void SetTraceId(const std::string& strTraceId);

private:
    ACTOR_TYPE m_eActorType;
    uint32 m_uiSequence;
    ev_tstamp m_dActiveTime;
    ev_tstamp m_dTimeout;
    Worker* m_pWorker;
    ev_timer* m_pTimerWatcher;
    std::string m_strActorName;
    std::string m_strTraceId;       // for log trace
    std::shared_ptr<Context> m_pContext;

    friend class WorkerImpl;
    friend class WorkerFriend;
    friend class Chain;
};
```

&emsp;&emsp;GetActorName()获取的是带名字空间的类名，熟悉Java的开发者都知道.class用起来很方便，GetActorName()就是Nebula框架的.class。

&emsp;&emsp;GetSequence()可以随时随地获取进程内唯一的一个32位无符号整数。

&emsp;&emsp;GetNowTime()比较近似地获取当前时间，在高并发的服务里频繁调用time()、gettimeofday()等函数对性能是有影响的，频繁调用GetNowTime()则不会有这样的问题。如果需要获取非常精确的时间，GetNowTime()是不适用的。

&emsp;&emsp;GetNodeId()/GetNodeType()/GetCustomConf()等用于获取当前节点、当前进程的一些重要信息，获取用户自定义配置信息。这些函数都非常有用，不用刻意去记，当需要用的时候再到Actor类里找，通常都有，毕竟Nebula是个久经多种业务生产环境使用的框架，功能比较全。再小提示一下，这些函数使得snowflake算法分布式生成唯一ID非常方便。

&emsp;&emsp;