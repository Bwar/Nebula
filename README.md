English | [中文](/README_cn.md)     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Bwar's blog](https://www.bwar.tech).

```
    _   __     __          __
   / | / /__  / /_  __  __/ /___ _
  /  |/ / _ \/ __ \/ / / / / __ `/
 / /|  /  __/ /_/ / /_/ / / /_/ /
/_/ |_/\___/_.___/\__,_/_/\__,_/
                                one-click installation 
```
# Nebula : a powerful framwork for building highly concurrent, distributed, and resilient message-driven applications for C++.
[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula) [![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)<br/>

1. [Overview](#Overview)
2. [Features](#Features)
3. [Getting Start](#GettingStart)
4. [Documentation](#Documentation)
5. [Depend on](#DependOn)
6. [Main Projects](#MainProjects)
7. [Todo list](#TODO)
8. [Change log](#ChangeLog)

<a name="Overview"></a>
## Overview 

Nebula is a flexible, high-performance business-oriented IoC distributed network framework developed in C++ language, designed for production environments. It supports multiple application layer communication protocols including proto3, http, https, and websocket. Nebula makes it easy to deploy a fast and high-performance distributed service with C++, while keeping the same server architecture and APIs. NebulaBootstrap provides out-of-the-box integration with Nebula service, but can be easily extended to serve other types of application.

Nebula is a production level framework and distributed solution project for instant messaging, data collection, real-time computing, message push and other applications, as well as web api services. There were production applications for instant messaging, data acquisition and real-time analysis on line now, and a recommendation engine application for a large user base will be born soon. By the way, using Nebula for toy-level projects is also good for learning network communication. Bwar welcomes more developers to join the Nebula project. Nebula is a proactor development framework(a framework-implemented proactor, not an operating system support). The IO-intensive application on nebula with be good performance.

Nebula can be used as a single high-performance TCP server, but building a cluster based on Nebula will be truly reflect its value. In order to build distributed service clusters quickly, Nebula Bootstrap cluster solutions including various types of services have been developed. 

<a name="Features"></a>
## Features
* Protocol communication such as http, protobuf, websocket, etc.
* Ssl connection encryption
* Microservices framework
* IoC container
* Dynamic service update
* Service registration and discovery
* Service monitoring
* Configuration management
* Routing
* Load balancing
* Circuit Breakers
* Leadership election and cluster state

<a name="GettingStart"></a>
## Getting Start
   Nebula was developed with C++11/C++14 standard, requires a compiler capable of the C++11-standard and at least gcc4.8(some C++14 features are replaced by C++11 standard when encountering a lower version of the compiler).
   We provides [NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap), which allows developers to build and deploy Nebula quickly. Nebula will be a framework that be widely used. A distributed solution based on NebulaBootstrap will make it easy to develop micro-service applications in C++.
   All dependencies will be automatically resolved in the following build steps.

   build step：
1. wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip
2. unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap; chmod u+x deploy.sh; chmod u+x deploy.sh
3. ./deploy.sh

   Run deploy.sh, the NebulaBootstrap distributed services were build completed. The reliance of Nebula was also automatically downloaded and compiled by deploy from the Internet before the construction of Nebula. The deploy path as follows:
* NebulaBootstrap
  + bin &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; server bin location。
  + build &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; build path，created by deploy.sh, if you do not need to build again, just delete it.(optional)。
  + conf &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; configuration path.
  + data &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; application data path. e.g. [Nebio](https://github.com/Bwar/Nebio) which is a data collect and real-time analysis application, write it's data to this path (optional).
  + lib &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; library path.
  + log &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; program logs path.
  + plugins &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; plugins path.
    - logic &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; plugins for logic server(optional).
  + script &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; script path. deploy.sh, startup.sh and shutdown.sh were depend on this path.
  + temp &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; temp file path(optional).
  - configure.sh &nbsp;&nbsp;&nbsp; run configure.sh for a simple configuration when deploy for the first time.
  - deploy.sh &nbsp;&nbsp;&nbsp;&nbsp; auto build and deploy.
  - shutdown.sh &nbsp;&nbsp;&nbsp;&nbsp; shutdown one or more server.
  - startup.sh &nbsp;&nbsp;&nbsp;&nbsp; startup one or more server.
  - README_cn.md              
  - README.md  

  build completed, you can start the server:
```
./configure.sh
./startup.s
```

The server should have started successfully now, startup.sh will print the server that had been started, If not, check logs for reason. Notice that the default configuration file of Nebula limits the number of connections per IP in a period. If you have a large amount of testing, you should check the configuration limit. If the server has been successfully started, testing with postman or curl.
```
# testing start with NebulaInterface only.
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:16003/hello

# testing start with NebulaInterface,NebulaLogic and NebulaBeacon.
curl -H "Content-Type:application/json" -X POST -d '{"name": "Nebula", "address":"https://github.com/Bwar/Nebula"}' http://${your_ip}:16003/hello_nebula
```
A simple testing can be start with a NebulaInterface only, and also can be start with NebulaBootstrap. NebulaBootstrap provided the a cluster HelloWorld, the testing will launch NebulaBeacon, NebulaInterface and NebulaLogic. This is a diagram of the cluster architecture:

![nebula_cluster](https://github.com/Bwar/NebulaBootstrap/blob/master/image/nebula_cluster.png?raw=true)

<a name="Documentation"></a>
## Documentation 
   The complete documentation for Nebula is available: [Nebula class reference](https://bwar.gitee.io/nebula) 
    
<a name="DependOn"></a>
## Depend on 
   * [protobuf](https://github.com/google/protobuf)
   * [libev](http://software.schmorp.de/pkg/libev.html) or [libev](https://github.com/kindy/libev)
   * [hiredis](https://github.com/redis/hiredis)
   * [crypto++](https://github.com/weidai11/cryptopp)
   * [http_parse](https://github.com/nodejs/http-parser) integrate into Nebula/src/util/http 
   * [CJsonObject](https://github.com/Bwar/CJsonObject) integrate into Nebula/src/util/json

<a name="MainProjects"></a>
## Main Projects
   * [NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)
   * [NebulaBeacon](https://github.com/Bwar/NebulaBeacon)
   * [NebulaInterface](https://github.com/Bwar/NebulaInterface)
   * [NebulaLogic](https://github.com/Bwar/NebulaLogic)
   * [NebulaMydis](https://github.com/Bwar/NebulaMydis)
   * [NebulaDbAgent](https://github.com/Bwar/NebulaDbAgent)
   * [NebulaLogger](https://github.com/Bwar/NebulaLogger)
   * [NebulaAccess](https://github.com/Bwar/NebulaAccess)
   * [NebulaDynamic](https://github.com/Bwar/NebulaDynamic)
   * [Nebcli](https://github.com/Bwar/Nebcli)

<a name="TODO"></a>
## Todo list 
   - Complete writing user guide before September 2019.
   - NebulaMydis Data Agency Service.
   - Developing an IM with the Nebula.

<a name="ChangeLog"></a>
## Change log 
#### v0.10
   - the plugin version control dynamically unloads and loads the instant validation feature.
   - optimize reflection dynamic creation of Actor.
#### v0.9
   - add Model
   - add Chain
   - simplify Context
   - optimize Actor
#### v0.8
   - compatible with gcc4.8 compiler.
   - add cpu affinity inorder to support cpu binding.
   - add dynamic library unload.
#### v0.7
   - add configuration management(check [Nebcli](https://github.com/Bwar/Nebcli) for detail).
#### v0.6
   - NebulaBeacon adds node status information query, registration center leader-fllower election.
   - NebulaInterface adds hello demo.
#### v0.5
   - add node info to worker the worker process terminated unexpectedly and restarted by the Manager.
   - ipv6 support.
#### v0.4
   - distributed log service test passing.
   - add https support.
   - add keep alive settings to http channel.
   - replace repeated http headers with proto3 map.
   - provides a symmetric encryption key setup interface for channel.
   - bug fix.
#### v0.3
   - rewrite with C++14
   - create actors by reflection (using template)
   - add distributed trace log
#### v0.2
   - the first runable version

<br/>


