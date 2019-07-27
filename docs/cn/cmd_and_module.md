### 应用Cmd类处理全部业务逻辑

&emsp;&emsp;Cmd和Module均为业务逻辑入口。在无须网络IO即可完成的场景里，Cmd和Module也可以完成全部处理逻辑，用于演示Nebula网络功能的demo就是一个Cmd或一个Module完成。先来看在[NebulaDynamic](https://github.com/Bwar/NebulaDynamic)中的CmdHello示例：

CmdHello.hpp:

```C++    
#include <actor/cmd/Cmd.hpp>

namespace logic
{

class CmdHello: public neb::Cmd, public neb::DynamicCreator<CmdHello, int32>
{
public:
    CmdHello(int32 iCmd);
    virtual ~CmdHello();

    virtual bool Init();
    virtual bool AnyMessage(
                    std::shared_ptr<neb::SocketChannel> pChannel, 
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody);
};

} /* namespace logic */    
```

&emsp;&emsp;CmdHello的头文件很简单，直接从neb::Cmd派生，实现Init()和AnyMessage()两个纯虚函数。事实上，所有的Cmd类定义都与CmdHello极为相似，复杂一点的Cmd也只是多定义几个成员函数给Init()或AnyMessage()调用而已。public neb::DynamicCreator<CmdHello, int32>用于给框架通过类名动态创建CmdHello，后续我们讲解动态创建时会详细描述，这里可以先理解为neb::DynamicCreator是固定不变的动态创建类，<CmdHello, int32>的第一个参数固定为当前派生类的类名CmdHello，后续参数数量和参数类型与当前派生类构造函数的参数个数和类型相同，CmdHello的构造函数只有一个int32型的参数。

CmdHello.cpp:

```C++
#include <fstream>
#include <sstream>
#include "util/json/CJsonObject.hpp"
#include "CmdHello.hpp"

namespace logic
{

CmdHello::CmdHello(int32 iCmd)
    : neb::Cmd(iCmd)
{
}

CmdHello::~CmdHello()
{
}

bool CmdHello::Init()
{
    return(true);
}

bool CmdHello::AnyMessage(
                std::shared_ptr<neb::SocketChannel> pChannel, 
                const MsgHead& oMsgHead, const MsgBody& oMsgBody)
{
    MsgBody oOutMsgBody;
    neb::CJsonObject oJson;
    oOutMsgBody.set_data("Nebula: hello!\n");
    SendTo(pChannel, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody);
    return(true);
}

} /* namespace logic */
```

&emsp;&emsp;CmdHello的源文件实现也很简单，在这个例子里只需要关注AnyMessage函数实现。这个函数是当从pChannel这个网络通信通道中收到一串字节流，Nebula框架将字节流解码成一个完整的消息（oMsgHead+oMsgBody）时，AnyMessage()被调用。CmdHello这个示例是在收到完整消息时直接回复一个消息，oOutMsgBody是将回复给客户端的消息体，oOutMsgBody.set_data("Nebula: hello!\n")是往消息体里填充内容。SendTo(pChannel, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody)是将消息通过请求来源通道pChannel原路返回给客户端。在Nebula框架里约定了整型cmd为奇数时是请求，为偶数时是响应，从AnyMessage()参数里进来的oMsgHead.cmd()必是奇数命令字，oMsgHead.cmd()+1是对应oMsgHead.cmd()的偶数响应命令字。oMsgHead.seq()为请求的seq，响应也以请求时的seq回复客户端。

&emsp;&emsp;这样一个简单的CmdHello就完成了网络通信的整个服务端代码。实现这样一个服务端代码只需要理解通信协议MsgHead和MsgBody，这个在[协议说明](protocol.md)章节里有详细说明。SendTo()函数是Actor基类的成员函数，在[Actor](actor.md)里有详细说明。基类neb::Cmd也十分简单，接下来我们讲解neb::Cmd的定义。

### neb::Cmd类定义

&emsp;&emsp;neb::Cmd类定义看起来跟上面的CmdHello很相似：

Cmd.hpp:

```C++
#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{

class Cmd: public Actor
{
public:
    Cmd(int32 iCmd)
        : Actor(Actor::ACT_CMD, gc_dNoTimeout),
          m_iCmd(iCmd)
    {
    }
    Cmd(const Cmd&) = delete;
    Cmd& operator=(const Cmd&) = delete;
    virtual ~Cmd(){};

    /**
     * @brief 初始化Cmd；重新加载配置
     * @note Cmd类实例初始化函数，大部分Cmd不需要初始化，需要初始化的Cmd可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Cmd的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Cmd名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     *     Init()需设计成可重入方法，因各服务框架在收到Beacon的重新加载配置指令后会执行每个
     * Cmd的Init()方法，所以必须保证Init()方法执行后没有副作用。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief 命令处理入口
     * @note 框架层成功解析数据包后，根据MsgHead里的Cmd找到对应的Cmd类实例调用将数据包及
     * 数据包来源pChannel传给AnyMessage处理。若处理过程不涉及网络IO之类需异步处
     * 理的耗时调用，则无需新创建Step类实例来处理。若处理过程涉及耗时异步调用，则应创建Step
     * 类实例，并向框架层注册Step类实例，调用Step.Emit()后即返回。
     * @param pChannel 消息来源通道
     * @param oMsgHead 数据包头
     * @param oMsgBody 数据包体
     * @return 命令是否处理成功
     */
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const MsgHead& oMsgHead,
                    const MsgBody& oMsgBody) = 0;

protected:
    int GetCmd() const
    {
        return(m_iCmd);
    }

private:
    int32 m_iCmd;
    friend class WorkerImpl;
};

} /* namespace neb */
```

&emsp;&emsp;neb::Cmd类是一个虚基类，且只有一个头文件。

### neb::Module类定义

&emsp;&emsp;neb::Module类也是业务逻辑入口，定义与Cmd类定义相似：

```C++
#include "codec/CodecHttp.hpp"
#include "labor/Worker.hpp"
#include "actor/Actor.hpp"
#include "actor/DynamicCreator.hpp"

namespace neb
{
class Module: public Actor
{
public:
    Module(const std::string& strModulePath)
        : Actor(Actor::ACT_MODULE, gc_dNoTimeout),
          m_strModulePath(strModulePath)
    {
    }
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    virtual ~Module(){};

    /**
     * @brief 初始化Module
     * @note Module类实例初始化函数，大部分Module不需要初始化，需要初始化的Module可派生后实现此函数，
     * 在此函数里可以读取配置文件（配置文件必须为json格式）。配置文件由Module的设计者自行定义，
     * 存放于conf/目录，配置文件名最好与Module名字保持一致，加上.json后缀。配置文件的更新同步
     * 会由框架自动完成。
     * @return 是否初始化成功
     */
    virtual bool Init()
    {
        return(true);
    }

    /**
     * @brief http服务模块处理入口
     * @param stCtx 消息来源上下文
     * @param oHttpMsg 接收到的http数据包
     * @return 是否处理成功
     */
    virtual bool AnyMessage(
                    std::shared_ptr<SocketChannel> pChannel,
                    const HttpMsg& oHttpMsg) = 0;

protected:
    const std::string& GetModulePath() const
    {
        return(m_strModulePath);
    }

private:
    std::string m_strModulePath;
    friend class WorkerImpl;
};

} /* namespace neb */
```

### 应用Module类处理全部业务逻辑

&emsp;&emsp;Module也可以完成全部处理逻辑，用于演示Nebula网络功能的demo就是一个Module完成。先来看在[NebulaDynamic](https://github.com/Bwar/NebulaDemo)中ModuleHello示例：

ModuleHello.hpp:

```C++
#include <string>
#include <actor/cmd/Module.hpp>

namespace demo
{

class ModuleHello: public neb::Module, public neb::DynamicCreator<ModuleHello, std::string>
{
public:
    ModuleHello(const std::string& strModulePath);
    virtual ~ModuleHello();

    virtual bool Init();

    virtual bool AnyMessage(
                    std::shared_ptr<neb::SocketChannel> pUpstreamChannel,
                    const HttpMsg& oHttpMsg);
};

} /* namespace demo */
```

&emsp;&emsp;ModuleHello的头文件很简单，直接从neb::Module派生，实现Init()和AnyMessage()两个纯虚函数。事实上，所有的Module类定义都与ModuleHello极为相似，复杂一点的Module也只是多定义几个成员函数给Init()或AnyMessage()调用而已。public neb::DynamicCreator<ModuleHello, std::string>用于给框架通过类名动态创建ModuleHello，后续我们讲解动态创建时会详细描述，这里可以先理解为neb::DynamicCreator是固定不变的动态创建类，<ModuleHello, std::string>的第一个参数固定为当前派生类的类名ModuleHello，后续参数数量和参数类型与当前派生类构造函数的参数个数和类型相同，ModuleHello的构造函数只有一个std::string型的参数。

ModuleHello.cpp:

```C++
#include "ModuleHello.hpp"
#include <util/json/CJsonObject.hpp>

namespace demo
{

ModuleHello::ModuleHello(const std::string& strModulePath)
    : neb::Module(strModulePath)
{
}

ModuleHello::~ModuleHello()
{
}

bool ModuleHello::Init()
{
    return(true);
}

bool ModuleHello::AnyMessage(
                std::shared_ptr<neb::SocketChannel> pChannel,
                const HttpMsg& oInHttpMsg)
{
    HttpMsg oHttpMsg;
    neb::CJsonObject oResponseData;
    oHttpMsg.set_type(HTTP_RESPONSE);
    oHttpMsg.set_status_code(200);
    oHttpMsg.set_http_major(oInHttpMsg.http_major());
    oHttpMsg.set_http_minor(oInHttpMsg.http_minor());
    oResponseData.Add("code", 0);
    oResponseData.Add("msg", "hello!");
    oHttpMsg.set_body(oResponseData.ToFormattedString());
    SendTo(pChannel, oHttpMsg);
    return(true);
}

} /* namespace demo */
```

&emsp;&emsp;ModuleHello的源文件实现也很简单，在这个例子里只需要关注AnyMessage函数实现。这个函数是当从pChannel这个网络通信通道中收到一串字节流，Nebula框架将字节流解码成一个完整的消息（oMsgHead+oMsgBody）时，AnyMessage()被调用。CmdHello这个示例是在收到完整消息时直接回复一个消息，oHttpMsg是将回复给客户端的消息体，oHttpMsg.set_body("")是往消息体里填充内容。SendTo(pChannel, oHttpMsg)是将消息通过请求来源通道pChannel原路返回给客户端。

&emsp;&emsp;这样一个简单的ModuleHello就完成了网络通信的整个服务端代码。

### 完整业务流程简析

&emsp;&emsp;上述Cmd、Module定义和例子是为了说明Cmd和Module两种组件的用途及使用方法，测试Nebula的功能demo有Cmd和Module基本上就可以了。然而，实际的业务应用并不是demo这么简单的，不过再复杂的业务以Actor组件来实现也不难，熟悉了Actor组件甚至能更好地帮助开发者做业务逻辑抽象出来。Nebula的Actor组件有7种，其中Cmd、Module、Step、Session是与Nebula生而俱来的组件，其中Cmd和Module可视作同一种组件，有了这几个组件就可以满足绝大部分复杂业务。Context、Model、Chain是Nebula发展过程中产生的组件，我们暂且先不用关注。

&emsp;&emsp;使用Actor组件的普遍业务流程如图：

![Actor业务流程](../image/static_chain.png)

&emsp;&emsp;如图所示，客户端请求从Cmd/Module进来，Cmd/Module创建一个步骤Step1完成异步IO操作，在Step1的回调之前，服务节点可以接收并处理其他客户端的请求。Step1完成后可能还需要Step2、Step3才完成整个业务流程，完成后将由最后一个Step给客户端发响应。在整个处理流程中可能需要暂时存储一些数据或共享一些数据，共享数据主要通过Session来实现。Context用于传递各Step所需的上下文，上下文传递也可以通过创建Step时的构造函数参数传递。

&emsp;&emsp;这是一个通过Actor组件实现业务逻辑的完整流程，再复杂的业务都可通过Actor组件的组合来实现，Step组件的章节还会有更多业务实现流程说明。无论何种业务流程都是通过上图的简单流程根据业务需要发展和衍生出来的，理解了这个简单的业务流程就容易理解后续讲解的业务流程。