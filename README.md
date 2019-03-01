English | [中文](/README_cn.md)     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Bwar's blog](https://www.bwar.tech).
# Nebula : An event driven asynchronous C++ framework
[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula) [![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)<br/>

1. [Overview](#Overview)
2. [License](#License)
3. [Getting Start](#GettingStart)
4. [Documentation](#Documentation)
5. [Depend on](#DependOn)
6. [Related Project](#RelatedProject)
7. [Todo list](#TODO)
8. [Change log](#ChangeLog)

<a name="Overview"></a>
## Overview 

Nebula is an event-driven TCP protocol network framework developed in C++ language. It supports multiple application layer communication protocols including proto3, http, https, and websocket. The purpose of developing the Nebula framework is to provide a fast and high-performance distributed service cluster based on C++.

Nebula can be used as a single high-performance TCP server, but building a cluster based on Nebula will be truly reflect its value. In order to build distributed service clusters quickly, Nebula Bootstrap cluster solutions including various types of services have been developed. 

<a name="License"></a>
## License 

MIT License

>  Copyright (c) 2018 Bwar
>
>  Permission is hereby granted, free of charge, to any person obtaining a copy
>  of this software and associated documentation files (the "Software"), to deal
>  in the Software without restriction, including without limitation the rights
>  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
>  copies of the Software, and to permit persons to whom the Software is
>  furnished to do so, subject to the following conditions:
>
>  The above copyright notice and this permission notice shall be included in
>  all copies or substantial portions of the Software.
>
>  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
>  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
>  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
>  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
>  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
>  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
>  THE SOFTWARE.

<a name="GettingStart"></a>
## Getting Start
   Nebula was developed with C++11/C++14 standard, and the compiler must fully support C++11(some C++14 features are replaced by C++11 standard when encountering a lower version of the compiler).
   Nebula was builded passing with gcc6.4 on centos6.5(upgrade binutils to 2.22 or later) and centos7.4.  
   We provides [NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap), which allows developers to build and deploy Nebula quickly. Nebula will be a framework that be widely used. A distributed solution based on NebulaBootstrap will make it easy to develop micro-service applications in C++.
   You must ensure that your system is installed with a fully C++11 compiler before you build, and all dependencies will be automatically resolved in the following build steps.

   build step：
1. wget https://github.com/Bwar/NebulaBootstrap/archive/master.zip
2. unzip master.zip; rm master.zip; mv NebulaBootstrap-master NebulaBootstrap
3. cd NebulaBootstrap
4. chmod u+x deploy.sh
5. ./deploy.sh

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
   The complete documentation for Nebula is available: [Nebula documentation](https://bwar.github.io/Nebula) and [Nebula wiki](https://github.com/Bwar/Nebula/wiki)
    
<a name="DependOn"></a>
## Depend on 
   * [protobuf](https://github.com/google/protobuf)
   * [libev](http://software.schmorp.de/pkg/libev.html) or [libev](https://github.com/kindy/libev)
   * [hiredis](https://github.com/redis/hiredis)
   * [crypto++](http://www.cryptopp.com)
   * [http_parse](https://github.com/nodejs/http-parser) integrate into Nebula/src/util/http 
   * [CJsonObject](https://github.com/Bwar/CJsonObject) integrate into Nebula/src/util/json

<a name="RelatedProject"></a>
## Related Project
   * [NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)
   * [NebulaBeacon](https://github.com/Bwar/NebulaBeacon)
   * [NebulaInterface](https://github.com/Bwar/NebulaInterface)
   * [NebulaLogic](https://github.com/Bwar/NebulaLogic)
   * [NebulaMydis](https://github.com/Bwar/NebulaMydis)
   * [NebulaDbAgent](https://github.com/Bwar/NebulaDbAgent)
   * [NebulaLogger](https://github.com/Bwar/NebulaLogger)
   * [NebulaAccess](https://github.com/Bwar/NebulaAccess)
   * [NebulaDynamic](https://github.com/Bwar/NebulaDynamic)

<a name="TODO"></a>
## Todo list 
   - NebulaMydis Data Agency Service.
   - Developing an IM with the Nebula.

<a name="ChangeLog"></a>
## Change log 
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


