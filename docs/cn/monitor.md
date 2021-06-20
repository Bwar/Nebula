&emsp;&emsp;Nebula提供了完善的数据上报和监控功能。

### 1. 多维度监控建议
&emsp;&emsp;运维监控可以从几方面考虑：

* 进程和端口监控，开发者可以自己写脚本做监控，也可以直接把nebula提供的启动脚本配置到crontab里。startup.sh 会通过端口检查进程是否存在，不存在则自动拉起。实际上nebula的进程模型就已十分稳定，业务逻辑基本都在worker进程里，worker进程挂了或者僵死都会被manager重新拉起，外围的启动脚本只是多一重几乎极少用到的保障。
* Beacon监控，通过nebcli可以获取到集群内各类型服务节点的实时状态，通过状态信息监控集群健康状态。如果信息不够，可以在增加插件获取更多数据，参考Beacon的ModuleAdmin插件可以了解增加监控信息很容易。
* 把NebulaLogger节点启动起来，集群内的日志会按日志级别和traceid发送到这个分布式日志收集节点，在logger里增加入库或写文件功能，再写个统一的日志扫描脚本做监控，比如扫描error数量，warning数量等。
* 用prometheus拉取监控数据，用grafana做监控展示。

### 2. 进程和端口监控

&emsp;&emsp;Nebula启动脚本startup.sh有检查端口启动进程功能，配置到crontab里，如果要到进程不存在的情况会被自动拉起。

&emsp;&emsp;如果需要将进程和端口信息监控上报到运维监控平台，可以自行编写shell脚本或python脚本完成。后续提到的Beacon监控和metrics监控可以覆盖脚本扫描进程和端口的功能。

&emsp;&emsp;Nebula支持进程模式和线程模式，其中进程模式是推荐使用。进程模式下的Nebula服务健壮性会更好，如果不是必须使用线程模式能满足功能需求的情况下都尽可能使用进程模式。

* 线程模式：Manager、Worker、Loader是一个进程的不同线程，Worker和Loader线程启动时由Manager线程创建并detach。
* 进程模式：Manager、Worker、Loader都是独立进程，Worker和Loader进程是启动时由Manager创建后独立运行，业务逻辑基本都在worker进程里，worker进程挂了或者僵死都会被manager重新拉起。

### 3. Beacon监控 

&emsp;&emsp;Nebula集群的节点会向Beacon上报节点信息数据，Beacon注册中心提供http接口用于查询这些信息，调用Beacon的http接口可以很方便地把这些信息用于报表监控。http接口请求和响应可以参考Beacon的ModuleAdmin实现。

&emsp;&emsp;我们还提供了一个用python开发的命令行工具[Nebcli](https://github.com/Bwar/Nebcli)可以很方便查询节点信息。Nebcli也是通过Beacon的http接口获取的数据。Nebcli的部分命令使用参考：

```
show nodes                                      # 查看在线节点信息
show nodes ${node_type}                         # 查看指定节点类型的在线节点信息
show node_report ${node_type}                   # 查看指定类型的节点工作状态（负载、收发数据量等）
show node_report ${node_type} ${node_identify}  # 查看指定节点类型指定节点工作状态
show node_detail ${node_type}                   # 查看指定类型的节点信息详情（IP地址、工作进程数等）
show node_detail ${node_type} ${node_identify}  # 查看指定类型指定节点的信息详情
show beacon                                     # 查看Beacon节点
```

### 4. 分布式日志监控 

&emsp;&emsp;把NebulaLogger节点启动起来，集群内的日志会按日志级别和traceid发送到这个分布式日志收集节点，在logger里增加入库或写文件功能，再写个统一的日志扫描脚本做监控，比如扫描error数量，warning数量等。日志上报到NebulaLogger节点是自动的，启动了NebulaLogger节点即可，无需任何其他操作。默认不上报DEBUG和TRACE日志，也不建议上报这两种日志，上报日志级别可以在服务配置文件中net_log_level配置。

```json
"log_levels": { "FATAL": 0, "CRITICAL": 1, "ERROR": 2, "NOTICE": 3, "WARNING": 4, "INFO": 5, "DEBUG": 6, "TRACE": 7 },
"log_level": 7,
"net_log_level": 5,
```

### 5. Prometheus&Grafana监控

&emsp;&emsp;Nebula框架层有连接数、收发包数、收发字节数统计，并且提供http接口查询，任意一个基于Nebula的服务只要启动了就可以通过接口获取到数据。这是在actor/cmd/sys_cmd/ModuleMetrics实现的。

框架层监控指标示例：

```
$ curl http://10.16.47.53:16380/status

nebula{app="fps-user", key="connect"} 269
nebula{app="fps-user", key="send_byte"} 70497873
nebula{app="fps-user", key="recv_byte"} 69727450
nebula{app="fps-user", key="send_num"} 63798
nebula{app="fps-user", key="recv_num"} 63936
nebula{app="fps-user", key="client"} 239
nebula{app="fps-user", key="load"} 269
```

业务层监控指标示例：

```
$ curl http://10.16.47.53:16380/metrics

fps_server{app="fps-user", key="connect"} 269
fps_server{app="fps-user", key="send_byte"} 70497873
fps_server{app="fps-user", key="recv_byte"} 69727450
fps_server{app="fps-user", key="send_num"} 63798
fps_server{app="fps-user", key="recv_num"} 63936
fps_server{app="fps-user", key="client"} 239
fps_latency{app="fps-user", key="HMGET", value_type="p999"} 9
fps_latency{app="fps-user", key="HMGET", value_type="p99"} 3
fps_latency{app="fps-user", key="HMGET", value_type="p95"} 1
fps_latency{app="fps-user", key="HMGET", value_type="p50"} 1
fps_server{app="fps-user", key="load"} 269
fps_req{app="fps-user", key="req", value_type="req_num"} 21478
fps_req{app="fps-user", key="req", value_type="success"} 21305
fps_req{app="fps-user", key="req", value_type="failed"} 0
```

&emsp;&emsp;框架层路径是/status，prometheus一般是从/metrics拉取数据了，为避免与业务自定义监控路径冲突，框架层用了/status。框架层的监控只要server启动好了就可以随时访问/status获取到。业务层监控可以参考actor/cmd/sys_cmd/ModuleMetrics自行实现。

&emsp;&emsp;这些监控的实现都是通过框架内置的数据上报功能完成的，上报的pb数据描述在Nebula/proto/report.proto里：

```proto
syntax = "proto3";
package neb;

message ReportRecord
{
    enum VALUE_TYPE
    {
        VALUE_ACC               = 0;
        VALUE_FIXED             = 1;
    }
    bytes key                   = 1;
    repeated uint64 value       = 2;
    string item                 = 3;
    VALUE_TYPE value_type       = 4;
}

message Report
{
    repeated ReportRecord records = 1;
}
```

&emsp;&emsp;Worker将Report上报到Manager（参考Worker::CheckParent()），Manager的actor/cmd/sys_cmd/CmdDataReport收到数据并存放到actor/session/sys_session/SessionDataReport中。ModuleMetrics通过获取SessionDataReport里的数据生成监控指标。

&emsp;&emsp;SessionDataReport会定时将Report上报到Beacon节点，如果在Beacon节点的Worker加载CmdDataReport（Manager默认LoadSysCmd()加载CmdDataReport，Beacon节点的Worker需在配置文件的boot_load中配置才会加载）即可获取到集群内所有节点的指标汇总。

