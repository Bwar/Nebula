[English](/README.md) | 中文      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Bwar的技术博客](https://www.bwar.tech).
# Nebula : 事件驱动型网络框架
[![](https://travis-ci.org/Bwar/Nebula.svg?branch=master)](https://travis-ci.org/Bwar/Nebula) [![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE)<br/>

1. [概述](#Overview)
2. [许可证](#License)
3. [编译](#Building)
4. [文档](#Documentation)
5. [依赖](#DependOn)
6. [开发任务](#TODO)
7. [版本历史](#ChangeLog)

<a name="Overview"></a>
## 概述 

Nebula是一个C\+\+语言开发的事件驱动型的TCP协议网络框架，它支持包括proto3、http、https、websocket多种应用层通信协议。开发Nebula框架的目的是提供一种基于C\+\+快速构建一个高性能的分布式服务集群。

Nebula可以作为单个高性能TCP服务器使用，不过基于Nebula搭建集群才能真正体现其价值。为了能快速搭建分布式服务集群，开发了包括各种类型服务的NebulaBootstrap集群解决方案。关于NebulaBootstrap的详细说明请参考[NebulaBootstrap](https://github.com/Bwar/NebulaBootstrap)。

<a name="License"></a>
## 许可证
> Copyright（c）2018 Bwar
>
> 特此免费授予任何人获得本软件及相关文档文件（“软件”）的副本，以无限制地处理本软件，包括但不限于使用，复制，修改和合并，发布，分发，再许可和/或销售本软件的副本，并允许本软件提供给其的人员遵守以下条件：
>
> 上述版权声明和本许可声明应包含在本软件的所有副本或主要部分中。
>
> 本软件按“原样”提供，不附有任何形式的明示或暗示保证，包括但不限于适销性，适用于特定用途和不侵权的保证。在任何情况下，作者或版权所有者都不承担任何索赔，损害或其他责任，无论是在合同，侵权或其他方面的行为，不论是由软件或其使用或其他交易引起或与之相关的行为。

<a name="Building"></a>
## 编译
Nebula在centos6.5（需升级binutils到2.22之后版本）和centos7.4上用gcc6.4编译通过。
![nebula_build_dir](docs/image/build_dir.png)

编译步骤：
  1. $ mkdir NebulaDepend
  2. 下载[依赖](#DependOn)并编译，编译完成后拷贝共享库到NebulaDepend/lib，拷贝头文件的文件夹到NebulaDepend/include。
  3. $ cd Nebula/src; $ make

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

<a name="TODO"></a>
## 开发任务
   - 2018年6月 增加https支持
   - 2018年8月 增加ipv6支持
   - 2018年10月 增加协程支持

<a name="ChangeLog"></a>
## 版本历史
#### v0.3
   - 用C++14标准改写整个项目
   - 用模板实现反射机制创建actor实例
   - 增加分布式追踪日志
#### v0.2
   - 第一个可运行并http测试通过的版本
<br/>


