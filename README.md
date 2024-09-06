### tcp打洞方案实现

#### tcpPunchingMaster
+ 服务端项目，go语言编写
+ 启动参数：./tcpPunchingMaster 100，后边的参数100是使用多少个客户端进行打洞。
    + 如果有超过100个客户端连接服务端，则会使用前100个进行打洞，后边50个不参与
    + 也只有连接服务端的客户端数量达到100个，才会开始下发打洞指令；服务端日志能看到当前有多少客户端连接
+ 编译命令：CGO_ENABLED=false GOOS=linux GOARCH=amd64 go build *.go  
+ 服务端项目根目录下自带一个已编译的可执行程序

#### tcpPunchingClient-final
+ 客户端项目，C语言编写
+ 启动参数：./tcpPunchingClient 123.123.123.123，后边的ip参数是服务端的ip地址
+ 编译命令：（arm32编译工具较差编译）arm-none-linux-gnueabihf-gcc --static -o tcpPunchingClient *.c
+ 客户端项目根目录下自带一个已编译的可执行程序

# TcpPunching
