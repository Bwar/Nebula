### Step定义
&emsp;&emsp;Step是最为核心的Actor组件，Nebula高性能的关键在于Step，IO密集型应用的逻辑都是围绕Step展开，暂且先把Step理解为一个操作步骤。Step起到C语言中异步回调函数的作用，却又比异步回调函数适用要简单得多。首先来看一下Step类图：

![Step类定义](../image/step.png)

&emsp;&emsp;如图所示，Step类分为三种PbStep、HttpStep和RedisStep，分别用在需要Nebula通信协议、Http通信协议和Redis通信协议的场景，其中PbStep应用最多，文档中的Step示例以PbStep为主。


Step.hpp:

```C++
#include "labor/Worker.hpp"
#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Chain;

class Step: public Actor
{
public:
    Step(Actor::ACTOR_TYPE eActorType, std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout);
    Step(const Step&) = delete;
    Step& operator=(const Step&) = delete;
    virtual ~Step();

    /**
     * @brief 提交，发出
     * @note 注册了一个回调步骤之后执行Emit()就开始等待回调。
     */
    virtual E_CMD_STATUS Emit(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL) = 0;

    /**
     * @brief 步骤超时回调
     */
    virtual E_CMD_STATUS Timeout() = 0;

protected:
    /**
     * @brief 执行当前步骤接下来的步骤
     */
    void NextStep(int iErrno = ERR_OK, const std::string& strErrMsg = "", void* data = NULL);

    uint32 GetChainId() const
    {
        return(m_uiChainId);
    }

private:
    ...
};

} /* namespace neb */
```

&emsp;&emsp;首先来看Step最重要最核心的两个函数Emit()和Callback()。Emit()用于发出网络IO，在发出IO前做开发者所需要的业务逻辑处理，包括组装用于向网络发出的数据包。Callback()在从Emit()发出的网络请求得到响应时由框架回调，响应数据通过Callback()的参数传递进来。

### Step是如何提高并发的

&emsp;&emsp;CPU与内存的速度远高于磁盘和网络，对IO密集型的应用而言，大部分时间消耗在IO等待上。把一次业务逻辑处理过程的CPU处理时间和IO等待时间放在一条时间轴上，CPU处理是时间轴上的一个个点，而IO等待是时间轴上的一个个线段。在多线程同步的处理模型里，是把一个个线段用其他线程处理了，在业务主线程里留下一个个点，一条时间轴可以排布非常多的点，并发量也就被提高了。然而，线程数量并不是越多越好的，线程有调度代价，当到了某个线程数量时，再增加线程数可能不仅不会提高并发量，反而会降低并发。事件驱动是在不增加线程的情况下把一个个线段移除出去只留下一个个点，理论上一条时间轴（一秒也可以看成一条时间轴）上可以排布多少个点并发就有多大。把线段从处理时间轴移除只保留两个端点只是为其他点腾出空间（时间），而不是把线段所需时间给消化掉了，所以事件驱动不会降低单个请求的处理时间，只是降低了请求之间相互等待的时间，可近似看作请求与请求之间无等待。

&emsp;&emsp;Step的Emit()和Callback()就是线段上的两个端点，Emit()和Callback之间的线段是IO等待时间，通过Step把这段时间移除出去只保留了Emit()和Callback()两个端点，Nebula实现了单个服务进程的高并发。Emit()和Callback()是Step最重要的两个函数，Emit()在Step中定义，Callback()因为参数不同分别在PbStep、HttpStep、RedisStep中定义，两个函数都是纯虚函数。Step还有一个纯虚函数Timeout()用于超时回调，当Emit()执行发出网络请求后超过指定超时时间未收到响应，Timeout()将会被调用。Step的派生类应选择从PbStep、HttpStep、RedisStep其中之一派生。

### Step调用链

&emsp;&emsp;一次客户端请求，服务端往往需要不止一次网络IO才能完成对客户端的服务，多次网络IO就有多个Step，多个Step有调用的先后顺序构成了Step调用链。Step调用链分为静态调用链和动态调用链。__静态调用链__是代码写完之后，调用顺序就固定下来了，哪怕有if条件或switch开关控制走不同的调用链分支也只是多了几个固定的调用顺序可以选择，从根本上说调用顺序就是已经固定下来了，这种调用链称为静态调用链。静态调用链的特点是调用顺序完全由编写Step业务代码的开发人员决定，想定义成什么样的调用顺序就写什么样的代码，框架是完全不会介入静态调用链的调用顺序，不会在一个Step执行完毕后自动调用它的NextStep()。__动态调用链__指的是通过配置文件将一系列相关（需要用到同一个Context或Session上下文）但又没有固定顺序（既没有在代码中制定NextStep，又没有在Step代码中创建Step并执行Emit（））的Step动态配置成调用链。动态调用链的特点是灵活，可以根据业务需要随时调整调用顺序。

&emsp;&emsp;静态调用链在Step的下文中有详细说明，动态调用链的使用在Chain组件的章节里说明。

### 构造Step

&emsp;&emsp;回过头来看看Step的构造函数：

```C++
Step(Actor::ACTOR_TYPE eActorType, std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout);

PbStep(std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout);
HttpStep(std::shared_ptr<Step> pNextStep = nullptr, ev_tstamp dTimeout = gc_dDefaultTimeout);
RedisStep(std::shared_ptr<Step> pNextStep = nullptr);
```

&emsp;&emsp;Step的构造函数有三个参数，因派生类是从PbStep、HttpStep、RedisStep其中之一派生，所以只关注这三个构造函数即可，Step构造函数的第一个参数是PbStep、HttpStep、RedisStep自动填充的。PbStep和HttpStep都有两个带缺省值的参数：pNextStep用于指定当前Step执行完之后将执行的Step；dTimeout用于指定当前Step的等待响应的超时时间。如果pNextStep不为空，意味着在构造当前Step时已经存在pNextStep这样一个实例，需要在正在构造的当前Step的Callback()函数里（return语句之前，这时当前Step事实上已经完成但尚未退出）显式调用NextStep()。NextStep()只用于静态调用链显式执行NextStep的Emit()，无论什么情况框架都不会自动调用NextStep()。pNextStep的缺省值为nullptr，因为很多情况下是用不到pNextStep的，对静态调用链而言当前Step的Callback()调用完是知道该调用哪个Step的，在需要调用时才创建，创建完立刻调用新Step的Emit()会更好更节省资源。dTimeout参数是Step等待响应的超时时间，只在有特殊需要时会用到，使用缺省值gc_dDefaultTimeout，实际上是在节点配置文件里step_timeout配置的值。RedisStep只有一个参数值，是因为RedisStep不需要作超时处理。
