[English](/README.md) | 中文      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Bwar的技术博客](https://www.bwar.tech).

```
    _   __     __          __
   / | / /__  / /_  __  __/ /___ _
  /  |/ / _ \/ __ \/ / / / / __ `/
 / /|  /  __/ /_/ / /_/ / / /_/ /
/_/ |_/\___/_.___/\__,_/_/\__,_/
                                一键安装部署
```

# Nebula : 一个强大的IoC网络框架，用于以C++快速构建高并发、分布式和弹性的消息驱动应用程序。
[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula) [![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)<br/>

1. [概述](#Overview)
2. [生产应用](#UseCase)
3. [功能](#Features)
4. [开始](#GettingStart)
5. [文档](#Documentation)
6. [依赖](#DependOn)
7. [相关项目](#RelatedProject)
8. [开发任务](#TODO)
9. [版本历史](#ChangeLog)
10. [交流与反馈](#Exchange)

<a name="Overview"></a>
## 概述 

&emsp;&emsp;Nebula是一个产线级的网络服务框架和分布式服务解决方案项目，适用于即时通讯、数据采集、实时计算、消息推送、接入网关、web后台服务等应用场景。

&emsp;&emsp;原生支持proto3、resp、http、https、http2、grpc、websocket多种应用层通信协议。

&emsp;&emsp;详细的文档，入门简单，扩展容易，开发效率高。

&emsp;&emsp;依赖的第三方库极少，开发和部署无成本。

&emsp;&emsp;基于channel、类actor模型异于传统rpc调用的无锁并发编程方式，进程&线程模型一键配置。

&emsp;&emsp;支持插件式动态加载，不停服务热更新更方便。

&emsp;&emsp;不仅是一个网络框架，还是一个十分通用的业务框架。选择Nebula，不必在网络框架之上再做业务框架封装。

&emsp;&emsp;代码结构清晰，可读性非常好。

<a name="UserCase"></a>
## 部分生产应用（按时间倒序）

* __计算机设备行业X公司智能设备数据采集服务__：基于http2协议通信向智能设备终端发连接，终端以http2流响应回传所采集的数据。
* __移动互联网行业O公司特征画像服务__：基于redis协议、grpc协议、http协议通信，带复杂数据逻辑的redis数据代理，既是redis服务端也是redis客户端，高峰tps 33万。
* __移动互联网行业O公司推荐引擎__：基于grpc协议和redis协议通信，计算密集型服务。
* __移动互联网行业O公司向量搜索服务__：基于grpc协议和http协议通信，计算密集型服务。
* __计算机设备行业X公司支付网关__：基于https协议通信，支付网关作为手持设备的https服务端和银行付款api的https客户端。
* __移动互联网行业O公司http接入网关__：基于http协议，包括条件转发、hash路由、服务熔断与恢复、过载保护、服务降级等功能。
* __游戏行业G公司即时通讯系统__：基于nebula协议和http协议通信，包括接入服务、逻辑服务、缓存代理服务、数据库代理服务、注册中心、配置中心等。
* __O2O行业J公司埋点数据采集和实时分析系统__：基于http协议和nebula协议通信，包括接入服务、逻辑服务、数据库代理服务、注册中心。
* __金融行业N公司即时通讯系统__：基于nebula协议和http协议通信，包括接入服务、逻辑服务、缓存代理服务、数据库代理服务、注册中心、配置中心、日志服务等。

<a name="Features"></a>
## 功能
* 支持http、protobuf、websocket等协议通信
* 支持ssl连接加密
* 微服务框架
* IoC容器
* 动态服务更新
* 服务注册
* 服务发现
* 服务监控
* 配置管理
* 动态路由
* 负载均衡
* 过载保护
* 故障熔断
* 故障检测和恢复
* 数据统计
* 分层服务
* 认证和鉴权
* 分布式日志跟踪
* 容易扩展第三方通信协议

<a name="GettingStart"></a>
## 开始
&emsp;&emsp;Nebula是个较完备的项目，也是一个你难得一见的依赖很少的项目，提供了许多开箱即用的功能，并提供了一键安装脚本，[NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)，让开发者可以快速部署和体验Nebula。相信部署和体验之后，你会对Nebula产生兴趣，这将会是一个可以广泛应用的框架，基于NebulaBootstrap提供的分布式解决方案可以很方便地用C++开发微服务应用。Nebula的相关项目就是学习和使用Nebula框架最好的例子。

* [高并发单机Server示例](docs/cn/nebula_server_demo.md)
* [分布式服务示例](docs/cn/nebula_distributed_demo.md)

* [安装部署说明](docs/cn/install.md)
* [Nebula工作原理](docs/cn/how_nebula_works.md)
* [配置说明](docs/cn/configuration.md)
* [服务监控](docs/cn/monitor.md)
* [协议说明](docs/cn/protocol.md)
* 开发组件说明
  * [Actor组件概述](docs/cn/actor_overview.md)
  * [Cmd和Module组件](docs/cn/cmd_and_module.md)
  * [Step组件](docs/cn/step.md)
  * [Session组件](docs/cn/session.md)
  * Context组件
  * Operator组件
  * Chain组件
  * [Actor类](docs/cn/actor.md)


![nebula_cluster](https://github.com/Bwar/NebulaBootstrap/blob/master/image/nebula_cluster.png?raw=true)

<a name="Documentation"></a>
## 文档
Nebula 完成的文档在 [Nebula参考手册](https://bwar.gitee.io/nebula)。

<a name="DependOn"></a>
## 依赖 
   * [protobuf](https://github.com/google/protobuf)
   * [libev](http://software.schmorp.de/pkg/libev.html) 或 [libev](https://github.com/kindy/libev)
   * [crypto++](https://github.com/weidai11/cryptopp)
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
   -  完成开发指南
   -  原生支持dubbo、mqtt等协议

<a name="ChangeLog"></a>
## 版本历史
#### v1.7.3
   - 修复熔断节点探测导致错误回调问题
   - 修复redis cluster执行批量写命令返回和asking命令bug
   - 修改channel创建方式，改由CodecFactory创建
#### v1.7.2
   - 修复codec bind channel循环引用问题
   - 修复CodecProto无包体通信问题
   - 启用连接保护配置
#### v1.7.1
   - 优化异步文件日志
   - SelfChannel增加seq
   - redis cluster客户端增加熔断和熔断恢复
   - 节点熔断bug修复
#### v1.7.0
   - 优化IO通信，编解码器插件化。
   - 移除Channel和Actor的shared_from_this，改用ChannelWatcher和ActorWatcher替代，以提高性能。
   - 移除动态加载插件的卸载功能（会降低性能）。
   - 裸数据编解码独立成编解码器。
   - 增加cassandra客户端编解码器。
   - 增加redis cluster密码校验支持。
   - 优化文件日志，提升写日志性能；增加异步文件日志。
#### v1.6.2
   - 断路器优化
   - http chunk解码、resp字符串解码、redis集群无可用节点bug修复
#### v1.6.1
   - http2分块响应通知改为作用于流而非连接
   - resp解码字符串遇分包时bug修复
#### v1.6.0
   - 增加用于Worker内通信的SelfChannel，完善actor模型
   - 增加服务状态监控和监控指标获取插件
   - 增加节点之间Manager到Manager通信功能 
   - 增加初始连接超时配置
   - 日志组件采用可变模板参数替代va_list，解决日志组件遇到%容易coredump问题
   - 增加一个端口多协议支持
   - 线程ID优化
   - Chain组件优化
   - http2动态表、流优先级bug修复
#### v1.5.0
   - 增加原生http2服务端和客户端支持  
   - 增加原生grpc服务端和客户端支持
   - 优化channel读写 
   - 增加绝对路径程序日志
#### v1.4
   - 以原生的CodecResp替代hiredis客户端
   - 增加redis cluster支持
   - 增加裸数据（RawData）传输支持
   - 增加日志实时flush可配置化
   - 增加IP地址组合节点类型支持
   - Dispatcher分发优化
   - bug修复
#### v1.3
   - 增加redis连接的非pipeline模式支持
   - 增加线程模式下Worker线程比Loader线程先启动，并把worker线程Id带到Loader
   - 初步合并@nebim写的CodecResp和CodecHttp2
   - 增加第三方cityhash
   - 更新CJsonObject
   - bug修复
#### v1.2
   - 增加延迟启动功能：因loader加载大量本地数据文件而延迟绑定端口提供服务并向Beacon注册。
   - 线程模式下Manager不再监控和重启不健康的worker和loader服务。
#### v1.1
   - 增加Worker的线程支持
#### v1.0
   - 从Manager和Worker类中分离出网络分发功能类Dispatcher和Actor创建及管理类ActorBuilder
   - Manager进程支持Actor类的使用，将Manager系统管理功能分离到Cmd类和Step类中
   - 增加Loader进程类型
   - 优化Actor通过反射动态创建实例
   - 优化http短连接和http数据收发
   - RedisChannel bug修复
   - 增加绑定IP配置
   - 增加压力测试相关接口
   - 增加编解码器自动转换
#### v0.10
   - 增加插件so版本控制动态卸载和加载即时生效功能
   - 优化反射动态创建Actor
#### v0.9
   - 增加Model模型组件
   - 增加Chain调用链组件
   - 简化Context上下文组件
   - 减少Actor继承层次，优化Actor相关代码
#### v0.8
   - 兼容gcc4.8编译器（从这个版本起无须另行安装5以上gcc版本，可以无障碍无等待地在个人机器上部署和测试，也为应用于生产铺平道路。之前Bwar的埋点数据采集和实时分析的生产项目Nebio是在服务器上安装了gcc6才部署的。）
   - 增加CPU亲和度设置以支持将Worker进程绑定CPU功能。(有人测试过繁忙的多核服务器，绑定CPU比不绑定CPU有20%左右的性能提升，现在Nebua可以让开发者自行选择是否绑定CPU)
   - 增加动态库（业务插件）卸载功能。（支持不停服务升级的重要功能）
#### v0.7
   - 增加配置管理，NebulaBeacon为配置中心，使用说明见命令行管理工具[Nebcli](https://github.com/Bwar/Nebcli)的get和set命令。
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

<a name="Exchange"></a>
## 交流与反馈
&emsp;&emsp;Bug、修改建议、疑惑都欢迎提在issue中，或加入qq群[809075299](点击链接加入群聊【Nebula框架技术交流】：https://jq.qq.com/?_wv=1027&k=5qL2ZKt)交流。

