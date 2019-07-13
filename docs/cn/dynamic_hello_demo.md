基于Nebula框架开发业务逻辑主要可能使用到的类包括Cmd、Module、Step、Session，这里给出一个从Cmd派生的最简单例子。<br/>
CmdHello.hpp:<br/>
```c++
#ifndef SRC_CMDHELLO_CMDHELLO_HPP_
#define SRC_CMDHELLO_CMDHELLO_HPP_

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

#endif /* SRC_CMDHELLO_CMDHELLO_HPP_ */

```
<br/>
CmdHello.cpp:    <br/>

```c++    

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
    oOutMsgBody.set_data("Nebula: hello!\n");
    SendTo(pChannel, oMsgHead.cmd() + 1, oMsgHead.seq(), oOutMsgBody);
    return(true);
}

} /* namespace logic */
```
<br/>
将CmdHello编译成Hello.so后，用在[NebulaLogic](https://github.com/Bwar/NebulaLogic)动态加载。编译CmdHello的Makefile如下：
<br/>

```makefile

CC = gcc
CXX = g++
CFLAGS = -g -O2 -fPIC
CXXFLAG =  -std=c++14 -O2 -Wall -ggdb -m64 -D_GNU_SOURCE=1 -D_REENTRANT -D__GUNC__ -fPIC -DNODE_BEAT=10.0

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
LIB3RD_PATH = ../../../NebulaDepend
NEBULA_PATH = ../../../Nebula
PLUGIN_PATH = ../../

VPATH = .
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(LIB3RD_PATH)/include \
       -I $(NEBULA_PATH)/include \
       -I $(PLUGIN_PATH)/src


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(NEBULA_PATH)/lib -lnebula \
           -L$(LIB3RD_PATH)/lib -lhiredis \
           -L$(LIB3RD_PATH)/lib -lcryptopp \
           -L$(LIB3RD_PATH)/lib -lprotobuf \
           -L$(LIB3RD_PATH)/lib -lev \
           -L$(SYSTEM_LIB_PATH) -lc -lrt -ldl

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))


TARGET = Hello.so

all: $(TARGET)

Hello.so:$(OBJS)
	$(CXX) -fPIE -rdynamic -shared -g -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(PB_OBJS) $(CMD_DEP_OBJS)
	rm -f $(TARGET)
```
编译完成后，部署到Logic，配置文件如下：<br/>
```json
{
    "node_type":"LOGIC",
    "//host":"系统内各Server之间通信绑定的IP（Server to Server）",
    "host":"192.168.157.138",
    "//port":"系统内各Server之间通信监听的端口",
    "port":16005,
    "//server_name":"异步事件驱动Server",
    "server_name":"neb_Logic",
    "//process_num":"进程数量",
    "process_num":1,
    "//worker_capacity":"子进程最大工作负荷",
    "worker_capacity":1000000,
    "//config_path":"配置文件路径（相对路径）",
    "config_path":"conf/",
    "//log_path":"日志文件路径（相对路径）",
    "log_path":"log/",
    "//max_log_file_num":"最大日志文件数量，用于日志文件滚动",
    "max_log_file_num":5,
    "//max_log_file_size":"单个日志文件大小限制",
    "max_log_file_size":20480000,
    "//io_timeout":"网络IO（连接）超时设置（单位：秒）小数点后面至少保留一位",
    "io_timeout":300.0,
    "//step_timeout":"步骤超时设置（单位：秒）小数点后面至少保留一位",
    "step_timeout":1.5,
    "log_levels":{"FATAL":50000, "ERROR":40000, "WARN":30000, "INFO":20000, "DEBUG":10000, "TRACE":0},
    "log_level":10000,
    "net_log_level":20000,
    "//beacon":"控制中心",
    "beacon":[
        {"host":"192.168.157.138","port":"16000"}
    ],
    "boot_load":{
        "cmd":[],
        "module":[]
    },
    "//refresh_interval":"刷新Server配置，检查、加载插件动态库时间周期（周期时间长短视服务器忙闲而定）",
    "refresh_interval":60,
    "dynamic_loading":[
        {
            "so_path":"plugins/logic/Hello.so", "load":true, "version":1,
            "cmd":[
                {"cmd":65531, "class":"logic::CmdHello"}
             ],
             "module":[
             ]
         }
    ],
    "//custom":"自定义配置，用于通过框架层带给业务",
    "custom":{
    }
}
```
