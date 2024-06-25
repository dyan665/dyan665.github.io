# this is reactor 服务器

[返回主页](../../README.md)

## 整体架构

> 基于reactor的服务器模型，主要包括了EventLoop类、Pooler类、Channel类、TcpConnection类、Accepter类、Connector类六个核心类。还包括基于此建立的TcpLcient和TcpServer类。

![reactor](./reactor.png)

## Pooler类
> Pooler类主要是对select、pool、epoll这几种IO复用模型接口的封装，主要功能包括保存关注的事件列表(使得能够通过`描述符fd`获取到对应的channel对象)、将事件通过接口`epoll_ctl`进行增删改、通过接口`epoll_wait`进行阻塞式等待事件到来或者超时返回。

## EventLoop类
> 核心类，作为各类

###线程委托

## Channel类
> 对`描述符fd`以及对应可读、可写、错误、关闭时的回调函数的封装。同时保存有EventLoop对象指针，通过该指针来将`fd`注册进Pooler中监听事件。

## Buffer类
>

## TcpConnection类


## Accepter类
> 对`server端`需要用到的接口进行的封装，主要是`创建socket`、`设置socket属性(比如地址、端口重用)`、`监听listen`、`接受连接accept`，其中accept接口则是包装为一个`Channel`对象，从而注册到`EventLoop`核心对象的Poller中进行事件监听。同时Accepter中保存有`newConnectionCallBack`回调函数，此函数是由`TcpServer`设置，目的是将新连接的文件描述符包装为`TcpConnection`保存在`TcpServer`中进行管理，包括设置读取到数据后的回调函数`messageCallback`、写数据完成时的回调函数`writeCompleteCallBack`、连接关闭时的回调函数`closeCallBack`、高水位线回调函数`highwaterCallBack`、低水位线回调函数`lowwaterCallBack`。

## Connector类


## TcpServer类


## TcpLcient类



