# this is reactor 服务器

[返回主页](../../README.md)

## 整体架构

> 基于reactor的服务器模型，主要包括了EventLoop类、Pooler类、Channel类、TcpConnection类、Accepter类、Connector类六个核心类。还包括基于此建立的TcpLcient和TcpServer类。

![reactor](./reactor.png)

## Pooler类（事件分离器）
> 内部类，主要是对select、pool、epoll这几种IO复用模型接口的封装，主要功能包括保存关注的事件列表(使得能够通过`描述符fd`获取到对应的channel对象,可通过`map`实现或者通过类似`epoll_event`内保存的用户数据指针实现均可)、将事件通过接口`epoll_ctl`进行增删改、通过接口`epoll_wait`进行阻塞式等待事件到来或者超时返回。

```cpp
class EventLoop;
class Channel;

class EPoller: noncopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    explicit
    EPoller(EventLoop* loop);
    ~EPoller();

    void poll(ChannelList& activeChannels);
    void updateChannel(Channel* channel);

private:
    void updateChannel(int op, Channel* channel);
    EventLoop* loop_;
    std::vector<struct epoll_event> events_;
    int epollfd_;
};
```

## EventLoop类
> 核心外部类，主要作用就是`1.调用事件分离器等待就绪事件；2.分发就绪事件给相应的事件处理器；3.调用处理器中的回调函数；4.处理其它任务队列中急需处理的任务以及超时事件；5.重复上述过程`。其作为各类的核心组件，管理着他们的生命周期，包括创建、删除时间分离器`Pooler`，创建、删除定时器队列`TimerQueue`，




### 线程委托




## Channel类
> 内部类，目前仅提供给`Acceptor`、`Connector`、`EventLoop`、`TcpConnection`使用，对`描述符fd`以及对应可读、可写、错误、关闭时的回调函数的封装。同时保存有EventLoop对象指针，通过该指针来将`fd`注册进Pooler中监听事件。

```cpp
class EventLoop;

class Channel: noncopyable
{
public:
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCallback;
    typedef std::function<void()> CloseCallback;
    typedef std::function<void()> ErrorCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void setReadCallback(const ReadCallback& cb)
    { readCallback_ = cb; }
    void setWriteCallback(const WriteCallback& cb)
    { writeCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }
    void setErrorCallback(const ErrorCallback& cb)
    { errorCallback_ = cb; }

    void handleEvents();

    bool polling;
    int fd() const
    { return fd_; }
    bool isNoneEvents() const
    { return events_ == 0; }
    unsigned events() const
    { return events_; }
    void setRevents(unsigned revents)
    { revents_ = revents; }

    void tie(const std::shared_ptr<void>& obj);

    void enableRead()
    { events_ |= (EPOLLIN | EPOLLPRI); update();}
    void enableWrite()
    { events_ |= EPOLLOUT; update();}
    void disableRead()
    { events_ &= ~EPOLLIN; update(); }
    void disableWrite()
    { events_ &= ~EPOLLOUT; update();}
    void disableAll()
    { events_ = 0; update();}

    bool isReading() const { return events_ & EPOLLIN; }
    bool isWriting() const { return events_ & EPOLLOUT; }

private:
    void update();
    void remove();

    void handleEventsWithGuard();

    EventLoop* loop_;
    int fd_;

    std::weak_ptr<void> tie_;
    bool tied_;

    unsigned events_;
    unsigned revents_;

    bool handlingEvents_;

    ReadCallback readCallback_;
    WriteCallback writeCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};
```

## 定时器Timer类

## 定时器队列TimerQueue类


## Buffer类
>

## TcpConnection类


## Accepter类
> 对`server端`需要用到的接口进行的封装，主要是`创建socket`、`设置socket属性(比如地址、端口重用)`、`监听listen`、`接受连接accept`，其中accept接口则是包装为一个`Channel`对象，从而注册到`EventLoop`核心对象的Poller中进行事件监听。同时Accepter中保存有`newConnectionCallBack`回调函数，此函数是由`TcpServer`设置，目的是将新连接的文件描述符包装为`TcpConnection`保存在`TcpServer`中进行管理，包括设置读取到数据后的回调函数`messageCallback`、写数据完成时的回调函数`writeCompleteCallBack`、连接关闭时的回调函数`closeCallBack`、高水位线回调函数`highwaterCallBack`、低水位线回调函数`lowwaterCallBack`。

## 连接器Connector类
> 内部类，作为客户端的连接器，负责创建socket、设置socket非阻塞等属性，以及最重要的调用`connect`接口连接服务器。

```cpp
class EventLoop;
class InetAddress;

class Connector: noncopyable
{
public:
    Connector(EventLoop* loop, const InetAddress& peer);
    ~Connector();

    void start();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    void setErrorCallback(const ErrorCallback& cb)
    { errorCallback_ = cb; }

private:
    void handleWrite();

    EventLoop* loop_;
    const InetAddress peer_;
    const int sockfd_;
    bool connected_;
    bool started_;
    Channel channel_;
    NewConnectionCallback newConnectionCallback_;
    ErrorCallback errorCallback_;
};
```


## TcpServer类
> 

## TcpLcient类
> ，同时包括重试等机制


