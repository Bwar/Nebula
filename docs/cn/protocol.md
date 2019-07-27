&emsp;&emsp;Nebula将网络消息分为请求（request）和响应（response）两类，事实上无论什么类型的网络消息也都可以归为请求或响应。

### Nebula通信协议
&emsp;&emsp;Nebula支持protobuf、http、websocket等通信协议，其中基于protobuf3设计的自定义通信协议是Nebula核心通信协议，我们称之为Nebula通信协议。Nebula通信协议用于Nebula分布式服务节点间的通信，也可用于与分布式服务外部的客户端通信。Nebula支持的http、websocket等协议都是在Nebula接入层解码后转成了Nebula通信协议在Nebula各节点间通信，并作为业务层可直接读写的数据结构。下面是Nebula协议的定义：

```protobuf
syntax = "proto3";

/**
 * @brief 消息头
 * @note MsgHead为固定15字节的头部，当MsgHead不等于15字节时，消息发送将出错。
 *       在proto2版本，MsgHead为15字节总是成立，cmd、seq、len都是required；
 *       但proto3版本，MsgHead为15字节则必须要求cmd、seq、len均不等于0，否则无法正确进行收发编解码。
 */
message MsgHead
{
    fixed32 cmd                = 1;           ///< 命令字（压缩加密算法占高位1字节）
    fixed32 seq                = 2;           ///< 序列号
    sfixed32 len               = 3;           ///< 消息体长度
}

/**
 * @brief 消息体
 * @note 消息体主体是data，所有业务逻辑内容均放在data里。req_target是请求目标，用于
 * 服务端接入路由，请求包必须填充。rsp_result是响应结果，响应包必须填充。
 */
message MsgBody
{
    oneof msg_type
    {
            Request req_target               = 1;                       ///< 请求目标（请求包必须填充）
            Response rsp_result              = 2;                       ///< 响应结果（响应包必须填充）
    }
    bytes data                  = 3;                    ///< 消息体主体
    bytes add_on                = 4;                    ///< 服务端接入层附加在请求包的数据（客户端无须理会）
    string trace_id             = 5;                    ///< for log trace

    message Request
    {
        uint32 route_id         = 1;                    ///< 路由ID
        string route            = 2;                    ///< 路由ID（当route_id用整型无法表达时使用）
    }

    message Response
    {
        int32 code            = 1;                     ///< 错误码
        bytes msg             = 2;                     ///< 错误信息
    }
}
```

&emsp;&emsp;Nebula协议由MsgHead和MsgBody构成，这是一种普遍使用的类TLV协议设计。固定为15字节的MsgHead。cmd命令字表示消息处理者对应的功能号，在Nebula里约定cmd为奇数时消息为request，cmd为偶数时消息为response，这样设计可以省去标识request和response的消息类型字段。cmd长度为32位4个字节，实际用作cmd功能的只有低位2个字节，高位2个字节用作压缩、加密算法标识和版本号等，这样做是为了尽量节省流量，因2字节的cmd已经可以表示32266（65535/2，0不能用作cmd，再排除Nebula保留的请求命令字500个）个功能，已足够业务使用。

&emsp;&emsp;MsgBody的data字段存储消息主体，任何自定义数据均可以二进制数据流方式写入到data。

&emsp;&emsp;msg_type用于标识该消息是请求还是响应（所有网络数据流都可归为请求或响应），如果是请求，则可以选择性填充Request里的route_id或route，如果填充了，则框架层无须解析应用层协议（也无法解析）就能自动根据路由ID转发，而无须应用层解开data里的内容再根据自定义逻辑转发。如果是响应，则定义了统一的错误标准，也为业务无关的错误处理提供方便。

&emsp;&emsp;add_on是附在长连接上的业务数据，框架并不会解析但会在每次转发消息时带上，可以为应用提供极其方便且强大的功能。比如，IM用户登录时客户端只发送用户ID和密码到服务端，服务端在登录校验通过后，将该用户的昵称、头像等信息通过框架提供的方法SetClientData()将数据附在服务端接入层该用户对应的长连接Channel上，之后所有从该连接过来的请求都会由框架层自动填充add_on字段，服务端的其他逻辑服务器只从data中得到自定义业务逻辑（比如聊天消息）数据，却可以从add_on中得到这个发送用户的信息。add_on的设计简化了应用开发逻辑，并降低了客户端与服务端传输的数据量。

&emsp;&emsp;trace_id用于分布式日志跟踪。分布式服务的错误定位是相当麻烦的，Nebula分布式服务解决方案提供了日志跟踪功能，协议里的trace_id字段的设计使得Nebula框架可以在完全不增加应用开发者额外工作的情况下（正常调用LOG4_INFO写日志而无须额外工作）实现所有标记着同一trace_id的日志发送到指定一台日志服务器，定义错误时跟单体服务那样登录一台服务器查看日志即可。比如，IM用户发送一条消息失败，在用户发送的消息到达服务端接入层时就被打上了trace_id标记，这个id会一直传递到逻辑层、存储层等，哪个环节发生了错误都可以从消息的发送、转发、处理路径上查到。

&emsp;&emsp;MsgHead和MsgBody的编解码实现见Nebula框架的[https://github.com/Bwar/Nebula/blob/master/src/codec/CodecProto.cpp](https://github.com/Bwar/Nebula/blob/master/src/codec/CodecProto.cpp)。

### Http通信协议
&emsp;&emsp;http协议应该是对外提供服务的最佳选择，作为一个通用网络框架，http协议自然是Nebula对外提供服务的最重要的协议。Nebula把http协议protobuf化，解析http文本协议并转化为HttpMsg在服务内部处理，应用开发者填充HttpMsg，接入层将响应的HttpMsg转换成http文本协议发回给请求方，不管服务端内部经过多少次中转，始终只有一次http文本协议的decode和一次http协议的encode。下面是http协议protobuf化的定义：

```protobuf
syntax = "proto3";
message HttpMsg
{
        int32 type                             = 1;            ///< http_parser_type 请求或响应
        int32 http_major                       = 2;            ///< http大版本号
        int32 http_minor                       = 3;            ///< http小版本号
        int32 content_length                   = 4;            ///< 内容长度
        int32 method                           = 5;            ///< 请求方法
        int32 status_code                      = 6;            ///< 响应状态码
        int32 encoding                         = 7;            ///< 传输编码（只在encode时使用，当 Transfer-Encoding: chunked 时，用于标识chunk序号，0表示第一个chunk，依次递增）
        string url                             = 8;            ///< 地址
        map<string, string> headers            = 9;            ///< http头域
        bytes body                             = 10;           ///< 消息体（当 Transfer-Encoding: chunked 时，只存储一个chunk）
        map<string, string> params             = 11;           ///< GET方法参数，POST方法表单提交的参数
        Upgrade upgrade                        = 12;           ///< 升级协议
        float keep_alive                       = 13;           ///< keep alive time
        string path                            = 14;           ///< Http Decode时从url中解析出来，不需要人为填充（encode时不需要填）
        bool is_decoding                       = 15;           ///< 是否正在解码（true 正在解码， false 未解码或已完成解码）
        message Upgrade
        {
            bool is_upgrade             = 1;
            string protocol             = 2;
        }
}
```

&emsp;&emsp;HttpMsg的编解码实现见Nebula框架的[https://github.com/Bwar/Nebula/blob/master/src/codec/CodecHttp.cpp](https://github.com/Bwar/Nebula/blob/master/src/codec/CodecHttp.cpp)。