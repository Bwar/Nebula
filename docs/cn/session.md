&emsp;&emsp;Session是仅次于Step的核心Actor组件，用于保存状态和数据。Cmd、Step等其他Actor组件也可以用Session来传递数据。在一些实时数据流应用中（比如Nebio）也适合把Session作最主要的组件使用。

### Session的方便性

&emsp;&emsp;Session是使用最为灵活的Actor组件，保存状态和数据是其最主要的用途，还有许多能想到的用途绝大部分都可用Session来实现。还是再提醒一句，使用灵活，但请别滥用。为什么Session可以如此灵活，因为只要知道SessionId就可以随时随地获取到指定的Session实例，而Session又是完全由用户自己定义，想如何用也就完全由用户自己控制。来看一个Session的使用例子：

```C++
std::shared_ptr<SessionOnlineNodes> pSessionOnlineNode = std::dynamic_pointer_cast<SessionOnlineNodes>(GetSession("beacon::SessionOnlineNodes"));
if (nullptr != pSessionOnlineNode)
{
    pSessionOnlineNode->AddBeaconBeat();
}
```

&emsp;&emsp;只要已知SessionId，在任何地方都可以调用GetSession()获取到对应Session实例。

### Session定义

&emsp;&emsp;Session类有一个子类Timer，两者的用途几乎完全一致。唯一区别在于Timer类总是会在指定的超时时间dSessionTimeout耗尽时被调用Timeout()回调函数；Session则会在每一次GetSession()调用时顺延其超时时间，只有在dSessionTimeout耗尽期间均未被访问过，Timeout()回调函数才会被调用。通俗点讲，Timer就是个定时器，时间一到，Timeout()一定会被调用；Session是会话，在用则不会超时，超过指定时间未被使用则会过期，Timeout()被调用。无论Session还是Timer，Timeout()被调用后，资源是应该被回收还是进入下一个计时周期完全取决于Timeout()的返回值，返回CMD_STATUS_RUNNING则继续下一个计时周期，返回其它值则进入结束状态，Session或Timer被回收。

！[Session类图]()

&emsp;&emsp;Session和Timer类定义：

```C++
class Session: public Actor
{
public:
    Session(uint64 ullSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    virtual ~Session();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

    const std::string& GetSessionId() const
    {
        return(m_strSessionId);
    }

    /**
     * @brief 检查Session内数据是否已加载
     * @note 为满足数据共享并确保同一数据在同一个Worker内只需从
     * 外部存储中加载一次，提供了IsReady()，SetReady()，IsLoading()，
     * SetLoading()四个方法。如果一个或若干个Step获取到一个已创建好
     * 的Session，则需先调用IsReady(this)判断数据是否就绪，若就绪则直接
     * 从Session中读取，若未就绪则调用IsLoading()判断数据是否正在加
     * 载，若正在加载则直接return(CMD_STATUS_RUNNING)（框架会在数据
     * 加载完毕后调用该Step的Callback），否则加载数据并且SetLoading()，
     * 数据加载完毕后SetReady()。
     * @param pStep 调用IsReady()方法的调用者Step的this指针，用于记录
     * 哪些Step依赖于Session的数据，在数据就绪时由框架主动调用
     * 依赖这些数据的Step回调而不需要等到超时才回调。
     */
    bool IsReady(Step* pStep);
    void SetReady();
    bool IsLoading();
    void SetLoading(); 

private:
    uint32 PopWaitingStep();

private:
    friend class WorkerImpl;
    bool m_bDataReady;
    bool m_bDataLoading;
    std::string m_strSessionId;
    std::queue<uint32> m_vecWaitingStep;
};

class Timer: public Session
{
public:
    Timer(uint64 ullSessionId, ev_tstamp dSessionTimeout = 60.0);
    Timer(const std::string& strSessionId, ev_tstamp dSessionTimeout = 60.0);
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    virtual ~Timer();

    /**
     * @brief 会话超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    /**
     * @brief Timer与Session的差异是通过覆盖Actor类的SetActiveTime()实现的，
     * 注意Timer的子类不可以再覆盖SetActiveTime()。
     */
    virtual void SetActiveTime(ev_tstamp dActiveTime)
    {
        ;
    }
};
```

&emsp;&emsp;构造Session或Timer需要两个参数，一个SessionId，一个超时时间。SessionId可以是64位无符号整数，也可以是字符串，最终SessionId都会转换成字符串来使用。如果需要永不超时，可以将SessionTimeout指定为gc_dNoTimeout。Session的私有成员除m_strSessionId之外还有好几个，开发者不需要理解。IsReady()，SetReady()，IsLoading()，SetLoading()四个保护成员方法会在少数场景中用到，注释中有比较充分的说明，本篇下文也会描述。

### Session应用

&emsp;&emsp;Session主要应用于状态和数据保存，下面结合几个具体场景更好清晰地描述Session的应用。

#### HTTP本地会话

&emsp;&emsp;当客户端浏览器通过HTTP1.1协议建立与Nebula的连接，服务端收到第一个请求时完成了客户端身份验证，将验证信息、浏览器cookie等以cookie为SessionId保存成一个Session。后续请求处理流程中GetSession(cookie)就可以获得整个会话数据。Session在整个HTTP生命周期都有效，每次请求均刷新超时起始时间，当连接中断或长时间没有请求，Session超时回收。

#### 推荐系统预估引擎的模型数据和字典

&emsp;&emsp;服务启动时将模型及其它字典加载到Session中，Timeout()用于检测本地模型文件或其它字典文件是否有更新，有更新则加载，不同的模型和字典更新频率会不一样，分别设置不同的超时时间避免无谓的检测操作非常合适。将模型数据和字典数据保存在Session里，可以很方便通过Cmd接收外部请求实时增量字典数据，有效解决大模型更新时引起的服务中断。当然，预估引擎模型和字典通常会分读、写两份数据，有专用线程做数据的更新，在数据更新完毕后才做读写指针互换。但实时增量更新带来的好处是显而易见的，提供了双内存操作的另一种很有价值的方式。

&emsp;&emsp;加载在Session里的数据可以很方便的给各算子和模型计算器通过SessionId读取到。

#### 即时通讯群组会话

&emsp;&emsp;像QQ群这样一个即时通信软件群组，在群消息通讯时需要用到许多信息：群名称、群头像、群创建时间、群成员数量、群成员在线情况、群主、群管理员、群成员昵称头像…… 这些信息在数据库或redis里都有存储，因群人数可能很多，即便去redis里查，代价也是比较大的。如果把这些信息都保存在Session里，在群活跃期间（假设有群消息等任何跟当前群相关的操作视为群活跃，20分钟内无任何操作则视为不活跃），需要用到这些信息的操作均可直接从本服务内存Session中获取到而无需远程请求redis，这将极大缩短了响应时间同时大幅降低了处理代价，缩短处理时间降低服务代价将直接带来每个请求服务质量和单机并发量的大幅提高。

&emsp;&emsp;把群相关数据保存在Session里，减少查询存储是有前提的，就是保证Session里的数据的完整性。如何保证Session数据的完整性不在这里展开，只给出两点提示：(1) 所有与同一个群相关的操作均由同一个服务节点完成；(2) 群相关数据的写操作先向存储系统（redis或消息队列，数据库太慢要异步写）发写请求，在存储系统返回成功响应后立即将刚写的数据更新到Session。如何让所有与同一个群相关的血操作均由同一个服务节点完成？使用Nebula构建的微服务系统实现非常简单，调用Actor基类的SendOriented()函数轻松解决。

&emsp;&emsp;群很容易产生并发，当一个未处于活跃状态的群的两个群成员恰好在同一时间发送消息或查看群信息时，请求又恰好同时路由到一个服务节点时，当前节点并不存在该群的Session，此时要去加载Session数据可能会导致多次加载。多次加载Session数据是不合理的，也违背了Session的设计原则。基于Nebula的服务是全异步的，所以不存在阻塞其他请求去等一个请求加载完Session数据这种降低并发的不合理的做法，也正因全异步“同时”到达的请求其实也是有先后顺序的，请求间可能有微秒甚至更细粒度的时间差异。这就是Session的四个保护成员方法IsReady()，SetReady()，IsLoading()，SetLoading()的用武之地了：

* 第一步：A请求发现群Session在当前服务节点不存在，创建群Session。
* 第二步：new一个StepA，执行StepA->Emit()向存储发起数据请求同时调用Session的SetLoading()设置Session数据加载中标识。
* 第三步：B请求发现群Session存在，new一个StepB执行StepB->Emit()调用Session的IsReady(this)，如果IsReady(this)返回true正常继续B请求的后续处理流程；如果IsReady(this)返回false，再调用IsLoading()，若IsLoading()返回true当前StepB->Emit()返回CMD_STATUS_RUNNING；若IsLoading()返回false，StepB->Emit()向存储发起数据请求同时调用Session的SetLoading()设置Session数据加载中标识。
* 第四步：A请求向存储发出读数据得到成功响应，StepA->Callback()被框架回调，在StepA的Callback()函数里将先数据存储到群Session里并调用该群Session的SetReady()，再继续A请求的后续逻辑（比如new StepA2并执行StepA2->Emit()）。
* 第五步：B请求StepB->Callback()被框架回调，StepA->Callback()的入参又给了StepB->Callback()（因为如果Session的IsLoading()返回false，将会由StepB->Emit()去加载数据，那样的话StepB->Callback()得到的入参跟StepA->Callback()的入参也还是一样的）。

&emsp;&emsp;这些步骤中只有一个发往存储系统的加载数据请求，框架得到存储系统的数据响应时给每个等待同一份数据的Step回调，同时将群Session的数据填充完毕，后续请求可以直接从群Session中获取数据，不再需要向存储系统发请求。

&emsp;&emsp;群Session应用场景有个流程图或时序图说明会更清晰，时间关系，没有画。