[English](/README.md) | 中文      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Bwar的技术博客](https://www.bwar.tech).
# Nebula : 事件驱动型网络框架
[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula) [![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)<br/>

1. [概述](#Overview)
2. [许可证](#License)
3. [开始](#GettingStart)
4. [文档](#Documentation)
5. [依赖](#DependOn)
6. [相关项目](#RelatedProject)
7. [开发任务](#TODO)
8. [版本历史](#ChangeLog)

<a name="Overview"></a>
## 概述 

Nebula是一个C\+\+语言开发的事件驱动型的TCP协议网络框架，支持包括proto3、http、https、websocket多种应用层通信协议。开发Nebula框架的目的是提供一种基于C\+\+快速构建一个高性能的分布式服务集群。Nebula自身核心代码只有万行左右（不计算proto文件生成的代码）。

Nebula可以作为单个高性能TCP服务器使用，不过基于Nebula搭建集群才能真正体现其价值。为了能快速搭建分布式服务集群，开发了包括各种类型服务的NebulaBootstrap集群解决方案。

Nebula从一个从2016年5月至今在生产环境稳定运行的IM底层框架Starship发展而来。Nebula跟Starship框架（也是Bwar一人独立开发）有20%左右的结构相似度，是基于Starship经验全新开发，可以认为Nebula(C++14)是Starship(C++03)的一个高级进化版本，具有Starship的所有优点，没有Starship的所有已发现的缺点，同时提供了更多高级功能。基于Nebula的第一个应用Nebio（埋点数据采集和实时分析项目）在2018年7月底上线并稳定运行，Bwar还准备开发基于Nebula的IM应用Nebim。

<a name="License"></a>
## 许可证
> Copyright（c）2018 Bwar
>
> 特此免费授予任何人获得本软件及相关文档文件（“软件”）的副本，以无限制地处理本软件，包括但不限于使用，复制，修改和合并，发布，分发，再许可和/或销售本软件的副本，并允许本软件提供给其的人员遵守以下条件：
>
> 上述版权声明和本许可声明应包含在本软件的所有副本或主要部分中。
>
> 本软件按“原样”提供，不附有任何形式的明示或暗示保证，包括但不限于适销性，适用于特定用途和不侵权的保证。在任何情况下，作者或版权所有者都不承担任何索赔，损害或其他责任，无论是在合同，侵权或其他方面的行为，不论是由软件或其使用或其他交易引起或与之相关的行为。

<a name="GettingStart"></a>
## 开始
&emsp;&emsp;Nebula以C++11/C++14标准开发，编译器必须完全支持C++11(部分C++14的特性在遇到较低版本的编译器时有预编译开关控制使用C++11标准替代),建议使用5以上gcc版本，推荐使用gcc6。Nebula目前只有Linux版本，暂无支持Linux之外的其他类UNIX系统的时间表。

&emsp;&emsp;Nebula在centos6.5（需升级binutils到2.22之后版本）和centos7.4上用gcc6.4编译和测试通过。同时Nebula也在[Travis CI](https://travis-ci.org/Bwar/Nebula)持续集成构建项目，构建结果可以直接通过项目首页的[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula)跳转过去查看。Travis CI的系统是Ubuntu，编译器是gcc6。

&emsp;&emsp;Nebula是个较大型项目（因为要构建一个生产用的分布式集群），有一些外部[依赖](#DependOn)，鉴于外部依赖的存在和框架本身较难测试，提供了[NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)，让开发者可以快速部署和体验Nebula。相信部署和体验之后，你会对Nebula产生兴趣，这将会是一个可以广泛应用的框架，基于NebulaBootstrap提供的分布式解决方案可以很方便地用C++开发微服务应用。

&emsp;&emsp;构建前必须保证你的系统已安装好完全支持C++11的编译器，除此之外的所有依赖都会在以下构建步骤中自动解决。

构建步骤：
1. wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip
2. unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap
3. cd NebulaBootstrap
4. chmod u+x deploy.sh
5. ./deploy.sh

&emsp;&emsp;执行deploy脚本后即完成了Nebula及NebulaBootstrap分布式服务的编译和部署，Nebula的依赖也由deploy在构建Nebula前自动从网上下载并编译部署。虽然不像autoconf、automake那样众所周知，但deploy脚本完成的不止是autoconf、automake的工作。deploy之后的目录结构如下：
* NebulaBootstrap
  + bin &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; server的bin文件存放路径。
  + build &emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; 构建路径，由deploy.sh生成，如果部署完后不需要再构建，可以直接删掉(可选)。
  + conf &emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 配置文件存放路径。
  + data &emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 数据文件存放路径，比如基于Nebula开发的页面埋点数据采集和实时分析[Nebio](https://github.com/Bwar/Nebio)项目，将采集的数据落地到这个路径（可选）。
  + lib &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; 运行所需的库文件存放路径。
  + log &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; 程序日志存放路径。
  + plugins &emsp;&emsp;&emsp;&emsp;&nbsp; 插件（动态加载的业务逻辑so）存放路径。
    - logic &emsp;&emsp;&emsp;&emsp;&nbsp; 逻辑Server插件存放路径，插件按server存放只是为了好区分，也可直接存放在plugins下，具体规则可自定义（可选）。
  + script &emsp;&emsp;&emsp;&emsp;&emsp; 脚本库存放路径，deploy.sh startup.sh shutdown.sh等脚本都需要依赖这个路径。
  + temp &emsp;&emsp;&emsp;&emsp;&emsp;&emsp; 临时文件存放路径(可选)。
  - configure.sh &emsp;&emsp;&emsp; 配置脚本，deploy之后第一次启动server之前先执行该脚本做简单的配置修改，也可以逐个配置文件打开直接修改。
  - deploy.sh &emsp;&emsp;&emsp;&emsp; 自动构建和部署脚本，自动下载并安装依赖，自动构建和部署，执行./deploy.sh --help查看帮助。
  - shutdown.sh &emsp;&emsp;&emsp;&nbsp; 关闭server，可以指定关闭一个或多个server，也可关闭所有server，不带参数时关闭所有server（需用户确认）。
  - startup.sh &emsp;&emsp;&emsp;&emsp; 启动server，可以指定启动一个或多个server，也可启动所有server。
  - README_cn.md              
  - README.md              

&emsp;&emsp;构建完成后，可以开始启动server了：
```
./configure.sh
./startup.sh
```
&emsp;&emsp;server应该已经启动成功了，startup.sh会打印已启动的server。如果没有启动成功，可以到log目录查看原因。执行grep "ERROR" log/*和grep "FATAL" log/* 先看看是否有错误，再到具体日志文件查看错误详情。注意，Nebula的默认配置文件对IP单位时间连接次数做了限制，如果在测试量较大发生莫名奇妙的问题，可以修改配置限制，通过查看日志中的WARNING信息通常有助于定位这种不是错误的“错误”。如果server已启动成功，那么可以用postman、curl等做测试，看看结果。
```
# 只启动NebulaInterface即可完成http的hello测试
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:16003/hello

# 启动NebulaInterface、NebulaLogic和NebulaBeacon完成分布式服务http的hello测试。
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:16003/hello_nebula
```
&emsp;&emsp;这个简单的测试可以只启动一个NebulaInterface即可完成，也可以启动分布式服务完成。NebulaBootstrap提供基于集群和单个Server的HelloWorld，基于集群的HelloWorld启动了NebulaBeacon、NebulaInterface、NebulaLogic三个server。下面是一张集群架构图：

![nebula_cluster](https://github.com/Bwar/NebulaBootstrap/blob/master/image/nebula_cluster.png?raw=true)

<a name="Documentation"></a>
## 文档
Nebula 完成的文档在 [Nebula documentation](https://bwar.github.io/Nebula)和[Nebula wiki](https://github.com/Bwar/Nebula/wiki)，持续更新中。

<a name="DependOn"></a>
## 依赖 
   * [protobuf](https://github.com/google/protobuf)
   * [libev](http://software.schmorp.de/pkg/libev.html) 或 [libev](https://github.com/kindy/libev)
   * [hiredis](https://github.com/redis/hiredis)
   * [crypto++](http://www.cryptopp.com)
   * [http_parse](https://github.com/nodejs/http-parser) 已集成到 Nebula/src/util/http
   * [CJsonObject](https://github.com/Bwar/CJsonObject) 已集成到 Nebula/src/util/json

<a name="RelatedProject"></a>
## 相关项目
   * [NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap) Nebula自动构建和部署
   * [NebulaBeacon](https://github.com/Bwar/NebulaBeacon) Nebula集群管理、注册中心、配置管理
   * [NebulaInterface](https://github.com/Bwar/NebulaInterface) Nebula集群http接入服务
   * [NebulaLogic](https://github.com/Bwar/NebulaLogic) Nebula集群逻辑服务
   * [NebulaMydis](https://github.com/Bwar/NebulaMydis) Nebula集群存储代理（redis）
   * [NebulaDbAgent](https://github.com/Bwar/NebulaDbAgent) Nebula集群数据库代理（mysql）
   * [NebulaLogger](https://github.com/Bwar/NebulaLogger) Nebula集群分布式日志服务
   * [NebulaAccess](https://github.com/Bwar/NebulaAccess) Nebula集群私有应用协议接入服务
   * [NebulaDynamic](https://github.com/Bwar/NebulaDynamic) Nebula集群动态加载插件
   * [Nebcli](https://github.com/Bwar/Nebcli) Nebula集群命令行管理工具

<a name="TODO"></a>
## 开发任务
   - NebulaMydis数据代理服务
   - 应用Nebula开发IM项目

#### v0.6
   - NebulaBeacon增加节点状态信息查询，注册中心主从高可用选举
   - NebulaInterface提供HelloWorld示例。
#### v0.5
   - 增加worker进程意外终止并被Manager重新拉起时的节点信息下发
   - 增加ipv6支持
#### v0.4
   - 分布式日志服务测试通过
   - 增加https支持
   - http通道增加keep alive设置
   - 用proto3中的map将http header替换掉原repeated数据类型
   - Channel增加设置对称加密密钥接口
   - 缺陷修复
#### v0.3
   - 用C++14标准改写整个项目
   - 用模板实现反射机制创建actor实例
   - 增加分布式追踪日志
#### v0.2
   - 第一个可运行并http测试通过的版本
<br/>


