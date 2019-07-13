### NebulaBootstrap
&emsp;&emsp;NebulaBootstrap是提供Nebula一键安装部署的一站式解决方案，不管是单机Server还是分布式服务均可通过NebulaBootstrap一键完成。NebulaBootstrap的核心是几个shell脚本，其目录结构就是Nebula项目的运行时目录结构。NebulaBootstrap项目会根据需要随Nebula项目的更新而更新，也可能保持不变。
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

&emsp;&emsp;conf路径上存放配置文件，通常有Server启动配置和业务自定义配置。Server启动配置文件名必须是Server在bin/路径下的二进制文件名加上.json后缀，否则服务不能正常启动。比如：bin/NebulaBeacon 对应的配置文件是 conf/NebulaBeacon.json。之所以规定得这么严格，是为了直观且方便管理，可以认为这就是Nebula部署规范之一。自定义的配置文件名和文件格式不作强制要求，不过建议用户自定义配置文件也使用json格式。

### 安装说明
&emsp;&emsp;NebulaBootstrap使原本依赖很少的Nebula部署起来更轻而易举，获取安装源->解压安装包，给脚本加可执行权限->一键安装。

```bash
# 获取安装源
wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip

# 解压安装包，给脚本加可执行权限
unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap; cd NebulaBootstrap; chmod u+x deploy.sh

# 一键安装
./deploy.sh
```

&emsp;&emsp;这里是为了方便安装和测试，把分布式的各个服务都部署在同一路径下，并且每个服务只启动了一个Worker进程。实际上，在生产环境部署时如果硬件资源允许，一般都是每台机器只部署一个服务，Worker数量与机器CPU核数相同。生产部署时，每个服务的目录结构都与NebulaBootstrap相同，build目录不应存在，plugins目录下没必要再创建子目录，deploy.sh脚本也可以删除。

