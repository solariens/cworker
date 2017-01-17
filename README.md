## 简介：  
    cworker是一个简单的服务器框架，cworker基于C语言多进程以及libevent事件轮询库，  
    开发者只要实现一个数据处理接口，便可以开发出自己的网络应用，例如Rpc服务、聊天室服务器、手机游戏服务器等。  
## 特性：  
    使用C/C++开发  
    支持多进程  
    标准输入输出重定向  
    支持基于事件的异步编程  
    守护进程化  
    支持TCP  
    回调接口支持自定义应用层协议  
    支持libevent事件轮询库，支持高并发  
    高性能  
## 用法：  
    初始化一个WebServer对象  
    WebServer webserver(const char *addr, int port);   
    设置server开启的进程数，同nginx的worker process  
    webserver.setWorkerProcess(int workerprocess);  
    指定回调函数处理数据，可以在回调函数自定义应用层协议  
    webserver.setDataHandler(char *(*dataHandler)(void *data))  
