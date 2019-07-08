### 安装单机Server Demo
&emsp;&emsp;Nebula提供一键安装部署，不管是单机Server还是分布式服务均一键完成，单机Server安装如下：

```bash
# 获取安装源
wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip

# 解压安装包，给脚本加可执行权限
unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap; cd NebulaBootstrap; chmod u+x deploy.sh

# 一键安装
./deploy.sh --demo
```

### 运行单机Server Demo
&emsp;&emsp;安装完成之后就可以启动Server并测试了，配置和启动过程同样是一键完成。

```bash
# 配置Server ip地址
./configure.sh

# 启动Server
./startup.sh

# 测试一下
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:15003/hello
```

&emsp;&emsp;这里给出的是用curl命令测试的方法，同样可以用postman等客户端测试。

### 单机Server简析
&emsp;&emsp;基于Nebula框架开发高并发单机Server是一件非常简单的事情。这个单机Server示例只有三个文件，当然，写成一个文件也是可以的，但为了清晰起见应独立成不同的文件。Nebula是个框架，编译成libnebula.so给业务开发使用，所以Nebula自身是没有main()函数的，各个基于Nebula的Server写个main()函数就完成了一个可运行的Server开发。先来看看这个不足10行的main()函数有多简单：

```C++
#include "util/proctitle_helper.h"
#include "labor/Manager.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "para num error!" << std::endl;
        exit(-1);
    }
    ngx_init_setproctitle(argc, argv);
    neb::Manager oManager(argv[1]);
    oManager.Run();
    return(0);
}
```

&emsp;&emsp;别看Nebula提供了NebulaBeacon、NebulaInterface、NebulaLogic等许多Server，实际上这些Server的main()函数及main()函数所在文件都是一样的。严格来说，这个main只需要三行：

```C++
// 设置程序名 
ngx_init_setproctitle(argc, argv);
// 定义Manager类实例
neb::Manager oManager(argv[1]);
//开始运行 
oManager.Run();
```

&emsp;&emsp;ModuleHello.hpp和ModuleHello.cpp是测试的业务逻辑代码，这个HelloWorld代码量也非常少。

ModuleHello.hpp:

```C++
#ifndef MODULEHELLO_HPP_
#define MODULEHELLO_HPP_

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

#endif /* MODULEHELLO_HPP_ */
```

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
                std::shared_ptr<neb::SocketChannel> pUpstreamChannel,
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
    SendTo(pUpstreamChannel, oHttpMsg);
    return(true);
}

} /* namespace demo */
```

