# Nebula : An event driven asynchronous C++ framework
[![Author](https://img.shields.io/badge/author-@Bwar-blue.svg?style=flat)](cqc@vip.qq.com)  ![Platform](https://img.shields.io/badge/platform-Linux-green.svg?style=flat) [![License](https://img.shields.io/badge/license-New%20BSD-yellow.svg?style=flat)](LICENSE)<br/>

Nebula is a BSD licensed, proto3-protocol event driven asynchronous c++ framework, it support multiple  protocol like protobuf, http, websocket. A high performance of distributed service clusters can be  easily constructed using Nebula.
    
## Depend on
   * [protobuf](https://github.com/google/protobuf)
   * [libev](http://software.schmorp.de/pkg/libev.html) or [libev](https://github.com/kindy/libev)
   * [hiredis](https://github.com/redis/hiredis)
   * [crypto++](http://www.cryptopp.com)
   * [http_parse](https://github.com/nodejs/http-parser) integrate into Nebula/src/util/http 
   * [cjson](http://sourceforge.net/projects/cjson/) now moved to [cjson](https://github.com/DaveGamble/cJSON). nebula CJsonObject base on a cjson version long time ago, cjson had been changed and integrated into Nebula/src/util/json
