### 安装Nebula分布式服务
&emsp;&emsp;Nebula提供一键安装部署，不管是单机Server还是分布式服务均一键完成，分布式服务安装如下：

```bash
# 获取安装源
wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip

# 解压安装包，给脚本加可执行权限
unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap; cd NebulaBootstrap; chmod u+x deploy.sh

# 一键安装
./deploy.sh
```

### 运行分布式服务Demo
&emsp;&emsp;安装完成之后就可以启动服务并测试了，配置和启动过程同样是一键完成。

```bash
# 配置Server ip地址
./configure.sh

# 启动Server
./startup.sh
```

&emsp;&emsp;server应该已经启动成功了，startup.sh会打印已启动的server。如果没有启动成功，可以到log目录查看原因。执行grep "ERROR" log/*和grep "FATAL" log/* 先看看是否有错误，再到具体日志文件查看错误详情。注意，Nebula的默认配置文件对IP单位时间连接次数做了限制，如果在测试量较大发生莫名奇妙的问题，可以修改配置限制，通过查看日志中的WARNING信息通常有助于定位这种不是错误的“错误”。如果server已启动成功，那么可以用postman、curl等做测试，看看结果。

```bash
# 测试一下
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:16003/hello
```

&emsp;&emsp;这里给出的是用curl命令测试的方法，同样可以用postman等客户端测试。

### Nebula分布式服务简析
&emsp;&emsp;基于Nebula框架开发分布式服务是一件非常简单的事情。Nebula提供了三个业务无关的Server：NebulaBeacon、NebulaInterface、NebulaLogic，通过这几个Server就可以构建分布式服务。Nebula是个框架，编译成libnebula.so给业务开发使用，所以Nebula自身是没有main()函数的，各个基于Nebula的Server写个main()函数就完成了一个可运行的Server开发。

&emsp;&emsp;基于Nebula框架的Server除负责Nebula集群管理、注册中心、配置管理的[NebulaBeacon](https://github.com/Bwar/NebulaBeacon)与Nebula框架关系非常紧密之外，其他如Nebula集群http接入服务[NebulaInterface](https://github.com/Bwar/NebulaInterface)、Nebula集群逻辑服务[NebulaLogic](https://github.com/Bwar/NebulaLogic)、Nebula集群存储代理（redis）[NebulaMydis](https://github.com/Bwar/NebulaMydis)、Nebula集群数据库代理（mysql）[NebulaDbAgent](https://github.com/Bwar/NebulaDbAgent) 、Nebula集群分布式日志服务[NebulaLogger](https://github.com/Bwar/NebulaLogger)、Nebula集群私有应用协议接入服务[NebulaAccess](https://github.com/Bwar/NebulaAccess)等都是为帮助开发者快速搭建分布式服务而提供的解决方案，同时也是Nebula框架的应用案例和教程。基于Nebula开发一个Server也就不到10行代码，如果觉得Nebula框架提供的Server不满足需求，可以不采用这些解决方案Server。比如，2018年7月起就应用于生产的埋点数据采集与实时分析项目[Nebio](https://github.com/Bwar/Nebio)就只用了NebulaBeacon和NebulaDbAgent，其他三个Server（Collect/Analyse/Aggregate）都是全新开发。

&emsp;&emsp;[NebulaDynamic](https://github.com/Bwar/NebulaDynamic) Nebula集群动态加载插件，Nebula的框架测试代码和各业务逻辑示例代码都会存放在这里。

