&emsp;&emsp;Nebula的配置文件为json格式，可读性、可扩展性和易维护性都很好，同时也非常方便通过配置中心远程管理。选择json格式配置文件还有很重要的一点，因为[CJsonObject](https://github.com/Bwar/CJsonObject)让使用json如使用C++自己的结构体那般方便，随心所欲（关注一下[CJsonObject](https://github.com/Bwar/CJsonObject)，这也许会是你见过的最为简单易用的C++json库）。

&emsp;&emsp;讲解配置文件前先明确几个概念：分布式服务集群（distributed service cluster）、节点（node）、接入（access）。每个启动的Server（可以理解为一个Manager进程）称为一个节点（node）；每个节点都会向一个Beacon或一组存在主备关系的多个Beacon节点发起注册，由同一个或一组Beacon管理的所有节点称为Nebula cluster。从Nebula cluster外部访问Nebula cluster需经由某一个或一组具有相同节点类型的节点，这些接收外部访问的动作称为接入，接收外部访问的节点称为接入节点。

&emsp;&emsp;Nebula项目提供的是配置文件的模板，每个基于Nebula的项目（如NebulaBeacon、NebulaInterface等）都有各自具体的配置文件，可以理解为Nebula提供的配置文件是类，基于Nebula个项目的配置文件是类的对象。如果基于Nebula开发了自己全新的Server服务，也可以给予Nebula的配置文件改一个属于Server自己的配置文件。先来看看配置文件的内容再来逐项配置说明：

```json
{
    "//node_type": "节点类型：ACCESS，LOGIC，PROXY，CENTER等，由业务层定义",
    "node_type": "ACCESS",
    "//access_host": "对系统外提供服务绑定的IP（Client to Server），若不提供对外服务，则无需配置",
    "access_host": "192.168.18.81",
    "//access_port": "对系统外提供服务监听的端口",
    "access_port": 9988,
    "//access_codec": "接入端编解码器，目前支持CODEC_PRIVATE(4),CODEC_HTTP(3),CODEC_PROTOBUF(2)",
    "access_codec": 4,
    "gateway": "113.102.157.188",
    "gateway_port": 9988,
    "//host": "系统内各Server之间通信绑定的IP（Server to Server）",
    "host": "192.168.18.81",
    "//port": "系统内各Server之间通信监听的端口",
    "port": 9987,
    "//server_name": "异步事件驱动Server",
    "server_name": "AsyncServer",
    "//worker_num": "进程数量",
    "worker_num": 10,
    "//with_loader":"是否启动loader进程",
    "with_loader":false,
    "//cpu_affinity":"是否设置进程CPU亲和度（绑定CPU）",
    "cpu_affinity":false,
    "//worker_capacity": "子进程最大工作负荷",
    "worker_capacity": 1000000,
    "//config_path": "配置文件路径（相对路径）",
    "config_path": "conf/",
    "//log_path": "日志文件路径（相对路径）",
    "log_path": "log/",
    "//beacon": "控制中心",
    "beacon": [
        { "host": "192.168.1.11", "port": 16000 },
        { "host": "192.168.1.12", "port": 16000 }
    ],
    "//max_log_file_num": "最大日志文件数量，用于日志文件滚动",
    "max_log_file_num": 5,
    "//max_log_file_size": "单个日志文件大小限制",
    "max_log_file_size": 20480000,
    "//permission": "限制。addr_permit为连接限制，限制每个IP在统计时间内连接次数；uin_permit为消息数量限制，限制每个用户在单位统计时间内发送消息数量。",
    "permission": {
        "addr_permit": { "stat_interval": 60.0, "permit_num": 1000000000 },
        "uin_permit": { "stat_interval": 60.0, "permit_num": 600000000 }
    },
    "//io_timeout": "网络IO（连接）超时设置（单位：秒）小数点后面至少保留一位",
    "io_timeout": 300.0,
    "//step_timeout": "步骤超时设置（单位：秒）小数点后面至少保留一位",
    "step_timeout": 1.5,
    "log_levels": { "FATAL": 0, "CRITICAL": 1, "ERROR": 2, "NOTICE": 3, "WARNING": 4, "INFO": 5, "DEBUG": 6, "TRACE": 7 },
    "log_level": 7,
    "net_log_level": 6,
    "//with_ssl": "SSL配置（可为空），路径为相对${WorkPath}的相对路径，公钥文件和私钥文件均为PEM格式",
    "with_ssl": {
        "config_path": "conf/ssl",
        "cert_file": "20180623143147.pem",
        "key_file": "20180623143147.key"
    },
    "//refresh_interval": "刷新Server配置，检查、加载插件动态库时间周期（周期时间长短视服务器忙闲而定）",
    "refresh_interval": 60,
    "load_config":{
        "manager":{
        },
        "worker":{
            "boot_load": {
                "cmd": [
                    { "cmd": 1001, "class": "neb::CmdHello" },
                    { "cmd": 1003, "class": "neb::CmdDbAgent" }
                ],
                "module": [
                    { "path": "neb/switch", "class": "neb::ModuleSwitch" },
                    { "path": "neb/hello", "class": "neb::ModuleHello" }
                ]
            },
            "dynamic_loading": [{
                    "so_path": "plugins/User.so",
                    "load": false,
                    "version": "1.0",
                    "cmd": [
                        { "cmd": 2001, "class": "neb::CmdUserLogin" },
                        { "cmd": 2003, "class": "neb::CmdUserLogout" }
                    ],
                    "module": [
                        { "path": "im/user/login", "class": "neb::ModuleLogin" },
                        { "path": "im/user/logout", "class": "neb::ModuleLogout" }
                    ],
                    "session":["im::SessionUser", "im::SessionGroup"],
                    "step":["im::StepLogin", "im::StepLogout", "im::StepP2pChat"],
                    "matrix":[],
                    "chain":[]
                },
                {
                    "so_path": "plugins/ChatMsg.so",
                    "load": false,
                    "version": "1.0",
                    "cmd": [
                        { "cmd": 2001, "class": "neb::CmdChat" }
                    ],
                    "module": [],
                    "session":[],
                    "step":[],
                    "matrix":[],
                    "chain":[]
                }
            ],
            "runtime":{
                "chains":{
                    "chain_1":["step1", "matrix1", ["step2A", "step2B", "step2C"], "step3", "matrix2"],
                    "chain_2":[]
                }
            }
        },
        "loader":{
        }
    },
    "//custom": "自定义配置，用于通过框架层带给业务",
    "custom": {}
}
```

&emsp;&emsp;配置文件自带了比较详细的注释，因json自身不支持注释，所以注释都是在需注释的key之前增加一个以json_key前加上“//”表示的key。配置文件里的必选配置项和可选配置项之间差异比较模糊，因为配置项本就很少，理解每项配置也很容易，给出必选可选提示反而会让人偷懒。如果不想先理解配置那么麻烦，想先以最快速度搭建服务体验一下，也没问题，[NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)提供了一键部署，可以很负责任地说Nebula绝对是极为少有依赖那么少部署那么容易的开源项目。配置文件模板所列的所有配置项都会在框架启动时自动读取，如果某个Server不需要这项配置，删掉并不会有任何副作用，如果是必需的配置项，Server启动时就会报错。

&emsp;&emsp;配置文件大体可以分为Server参数配置、控制中心配置、动态加载配置、运行时配置和自定义配置五部分。日志级别和动态加载配置、运行时配置、自定义配置都支持运行时修改，一个刷新周期refresh_interval后生效，如果是通过Beacon配置中心修改则修改成功后立即生效。

#### Server参数配置
* node_type 节点类型，由业务层定义，框架把节点类型当字符串处理，用于标识若干（数量不限）服务节点，节点类型字符串建议全大写字母。在Beacon配置节点订阅关系就是通过此项节点类型配置来实现。比如，有3个Server的节点类型配置为“ACCESS”，另有6个Server的节点类型配置为“LOGIC”，在Beacon配置了ACCESS类型的节点订阅LOGIC类型节点信息，那么当有任意LOGIC类型上线、下线时，所有ACCESS类型的节点都会收到通知，ACCESS收到通知后会自动更新内存里存储的LOGIC节点信息。
* access_host 对系统外提供接入服务绑定的IP（Client to Server），若当前节点不提供对外接入服务，则可以直接删除这项配置。
* access_port 对系统外提供接入服务监听的端口，若当前节点不提供对外接入服务，则可以直接删除这项配置。
* access_codec 对外接入服务的编解码器。目前支持CODEC_PRIVATE(4),CODEC_HTTP(3),CODEC_PROTOBUF(2)，且只能配置一种编解码器，后续有可能在同一个端口提供多种协议编解码服务。若当前节点不提供对外接入服务，则可以直接删除这项配置。
* gateway 网关服务地址，用于客户端路由和负载均衡。gateway和gateway_port提供的是预先通过请求某个接口获取接入服务地址的负载均衡方式，因每个节点的负载都可以从Beacon获取到，客户端发业务请求前先通过一个固定接口查询需将正式请求发往哪个（些）接入节点，得到接入节点地址后再发业务请求。为什么不直接返回access_host、access_port？因为这两个是节点实际绑定的地址和监听的端口，外部不一定能直接访问（很可能也是内网IP和端口），比如需要通过交换机或防火墙，而交换机或防火墙是通过端口映射的方式到access_host、access_port的，那么客户端需要获取的是映射前暴露在外网的地址和端口，此时gateway和gateway_port的作用就体现出来了。假设接入服务前端还有一层类似LVS的外部负载均衡服务，只需把access_host、access_port配置到负载均衡服务即可，gateway和gateway_port不会用到。如果access_host和access_port是外部客户端可以直接访问的，则gateway配置的地址与access_host相同，gateway_port与access_port相同。
* gateway_port 网关服务端口。
* host 集群内部通信地址。
* port 集群内部通信端口。host:port组成的字符串在集群内唯一标识一个节点，用作管理层面（与Beacon通信）名字服务的节点名字。host:port.workerindex组成的字符串在集群内唯一标识一个节点的一个Worker进程，用作数据层面（也即业务层面，除Beacon之外的节点间通信）名字服务的节点Worker名字。
* server_name 节点Server进程名，方便在服务器中标识和管理，在集群管理和节点通信中都不会用到。通俗一点讲，这是给人用的不是给机器用的。
* worker_num Worker进程数量，每个节点由一个Manager进程和若干个Worker进程构成。通常，如果某台机器只部署了一个Nebula服务并且主要是给这个服务使用的，为了更充分使用机器资源，将worker_num配置成与cpu核数相同。
* with_loader 是否启动loader进程。Loader进程用于做本地数据存储，大部分IO密集型的应用不会用到，所以默认不会启动Loader进程。
* worker_capacity 进程容量，用于过载保护。进程负载 = Channel数量 * 系数 + Step数量 * 系数。这个计算公式会根据需要和合理性做调整。当进程负载达到进程容量限制时会拒绝新的连接。
* cpu_affinity CPU亲和度，为true时，Worker进程会均匀地绑定到CPU核。默认为false，不绑定。
* config_path 配置文件存储路径，相对于NEBULA_HOME的路径。
* log_path 日志文件存储路径，相对于NEBULA_HOME的路径。
* max_log_file_num 最大日志文件数量，用于日志文件滚动，超出这个数量的日志文件将会被直接删除。
* max_log_file_size 单个日志文件大小限制，用于日志文件滚动，超出这个限制将滚动日志，生成一个新的日志文件。
* log_levels 日志级别枚举，仅用于给log_level和net_log_level配置时参考。
* log_level 本地文件日志级别。框架级别的调试日志用的是LOG4_TRACE(日志量很大，部署生产时请不要打开TRACE级别日志)，建议业务的调试日志用LOG4_DEBUG。
* net_log_level 发到LOGTRACE日志跟踪服务的网络日志级别。
* permission 连接或消息发送频率限制。addr_permit为连接限制，限制每个IP在统计时间内连接次数，超出permit_num次数限制的连接会被直接拒绝；uin_permit为消息数量限制，限制每个用户在单位统计时间内permit_num发送消息数量，超出限制数量的消息会被直接丢弃。stat_interval到了之后重新计算。
* io_timeout 网络IO（连接）超时设置（单位：秒）小数点后面至少保留一位，用于触发连接的有效性检查。如果在到达超时时间前连接有数据收或发过，则从最后一次收发数据时间开始重新计算超时时间；如果超时时间到达却一直没有数据收发过，有三种处理情况：(1) 需要做应用层心跳检查，自动发送心跳包，心跳包得到响应，连接得以保持，重新计算超时时间；(2) 需要做应用层心跳检查，自动发送心跳包，心跳包未得到响应，立即断开连接，回收连接所分配资源；(3) 无须做应用层心跳检查，立即断开连接，回收连接所分配资源。
* step_timeout 步骤超时设置（单位：秒）小数点后面至少保留一位，用于请求发出后等待响应的默认超时等待。在代码层可以为每一个发出的请求设置等待超时，通常为Step类的最后一个参数，如果这个参数为缺省值，则配置文件里的step_timeout会作为这个Step发出请求后等待响应的超时时间。
* with_ssl 配置接入服务的对外连接是否需要SSL传输加密，如果需要则配置ssl配置文件路径和相关文件名。
* refresh_interval 刷新Server配置，检查、加载配置或加载卸载插件动态库时间周期。
* load_config 功能模块加载。功能模块分manager、worker、loader进程的功能模块，分别加载到对应类型的进程里。
* boot_load 服务启动加载的处理模块入口。Nebula框架会被编译成动态库 libnebula.so，基于Nebula框架的Server需要写个main函数并编译链接成一个二进制文件，比如NebulaBeacon，还有一些不需要做动态加载的模块会一起编译链接到这个二进制文件里，而这些模块是Nebula框架未知的（libnebula.so早于任何一个server二进制文件存在），boot_load就是为这些模块加载使用的。boot_load里的配置说明见下文描述动态加载配置说明。

#### 控制中心配置
* beacon 控制中心地址和端口，有几个控制中心节点就配置几条。如果无须控制中心，则配置为空。

#### 动态加载配置
&emsp;&emsp;动态加载配置dynamic_loading是一个json数组，每个数组元素都是一个SO动态库插件。每个插件的配置项说明如下：

```json
    "dynamic_loading": [{
            "so_path": "plugins/User.so",
            "load": false,
            "version": 1,
            "cmd": [
                { "cmd": 2001, "class": "neb::CmdUserLogin" },
                { "cmd": 2003, "class": "neb::CmdUserLogout" }
            ],
            "module": [
                { "path": "im/user/login", "class": "neb::ModuleLogin" },
                { "path": "im/user/logout", "class": "neb::ModuleLogout" }
            ],
            "session":["im::SessionUser", "im::SessionGroup"],
            "step":["im::StepLogin", "im::StepLogout", "im::StepP2pChat"],
            "model":[]
        }
    ]
```

* so_path 动态库路径，包含文件名，是相对于NEBULA_HOME的路径。
* load 表明该插件是否需要被服务加载。服务运行过程中，把load从true改为false，在下一个refresh_interval到来时，该插件会被卸载；把load从false改成true，在下一个refresh_interval到来时，该插件会被加载。
* version 插件版本。如果load为true，当版本发生变更时，在下一个refresh_interval到来时，该插件会被卸载然后重新加载。因插件在被正在运行的服务加载时，直接替换so文件会导致服务coredump，所以虽然version这个功能存在，但并无有意义的作用。如果要升级插件，通常是先把load改为false，等待一个refresh_interval，再替换so文件，把load改为true。这样做插件升级，虽然不用重启服务，但实际上服务对应插件的功能是会有一到两个refresh_interval的中断，其他插件服务不受影响。
* cmd 以命令字作为入口的模块定义。这是一个json数组，意味着一个动态库里可以有多个功能模块，通常相关联的功能模块会被编译链接到同一个插件动态库里。当然，每个功能模块自成一个插件动态库也是可以的。
  + cmd 命令字。
  + class 类名，如果类被定义在某个名字空间里，则类名必须带上名字空间。插件被动态加载时，框架会通过类名反射机制创建对应Cmd类实例。
* module 以url location路径作为入口的模块定义，用于http模块接入。这也是一个json数组，一个动态库里可以有多个功能模块，通常相关联的功能模块会被编译链接到同一个插件动态库里。当然，每个功能模块自成一个插件动态库也是可以的。
  + path url路径，不含schema和host、port部分。
  + class 类名，如果类被定义在某个名字空间里，则类名必须带上名字空间。插件被动态加载时，框架会通过类名反射机制创建对应Module类实例。
* session Session类名数组，配置该插件内所有Session类名，类名需带上名字空间。插件卸载时，所有插件内的符号（在这里体现为各动态创建的类对象）均需要先释放再调用unload_so，所以类名如果配置不全将可能会导致卸载不完整，服务有可能会在后续执行中coredump。
* step Step类名数组，配置该插件内所有Step类名，类名需带上名字空间。作用同上。
* model Model类名数组，配置该插件内所有Model类名，类名需带上名字空间。作用同上。

#### 运行时配置
&emsp;&emsp;运行时配置是为了提供随时调整服务的功能，在许多业务场景里边不会用到。当前的运行时配置只有调用链配置。

```json
"runtime":{
        "chains":{
            "chain_1":["step1", "matrix1", ["step2A", "step2B", "step2C"], "step3", "matrix2"],
            "chain_2":[]
        }
    }
```

* chains 调用链（调用链的概念和使用方法详见Actor组件说明），调用链的配置是一个类似于数组的配置，可以理解为chains里存储的是一个map，map的key为一个调用链标识，map的value为调用链的链块配置。“chain_1”、“chain_2”只是用于标识一个调用链，名字由开发者自行定义，无特殊含义也无任何限制。chain map的value部分配置的是调用链的实际调用顺序，这是一个一维或二维的json数组，每个数组元素只能是Step类或Model类，第一维的元素是串行关系，第二维的元素是并行关系，并行关系的元素只能是Step类。如示例“chain_1”所示，调用链首先调用step1，在step1完成后调用matrix1，matrix1完成后同时调用step2A、step2B、step2C，在这三个Step都完成后才调用step3，step3完成后整个调用链完成。如果调用链只有串行关系，只配置一维即可。

#### 自定义配置
&emsp;&emsp;自定义配置是业务层所需的配置，通常会独立定义配置文件存放在conf路径下。这里的自定义配置指的不是那种业务独立定义的配置文件，而是以“custom”为key的json嵌入到Nebula框架配置文件中的自定义配置。这样配置有什么好处？因为所有的业务代码类都是Actor的派生类，把自定义配置嵌入到框架的配置文件中，只需调用GetCustomConf()方法即可获得配置内容，这个方法是Actor类的成员方法，读取配置就跟访问类自己的成员一样随时随地随心所欲。虽然把自定义配置嵌入到Nebula框架配置文件里非常方便，但也不要滥用，如果什么配置都往这里放会使得框架配置文件变得非常大，可读性和可维护性都变差。建议把许多业务功能模块都会用到或使用频率非常高，配置量不是太大的配置嵌入到框架配置文件里，而只有少数模块会用到，又或使用频率低，或配置内容非常庞大的应独立成其他自定义配置文件。非json格式的配置也应独立成自定义配置文件。

```json
"custom": {}
```

