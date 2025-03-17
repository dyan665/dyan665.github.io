# rest sdk（简单的http1下的CRUD rest sdk）

[返回主页](../../README.md)

# 原理
> > 基于libevhtp来实现，libevhtp是一个基于libevent的http sdk，能够对uri定义回调函数，并传递请求的一些信息（比如uri，header、body等），此sdk的目的就是只针对简单的CRUD，将用户定义的回调函数包装在一起，然后注册进libevhtp中，统一进行回调处理。
> > 回调函数采用统一的格式，会将header、body等包装进REST_REQUEST或者REST_RESPONSE，只通过api接口暴露给用户，用户通过接口获取或者增删修改其中的信息，最后通过Handle中的libevhtp的函数统一发送出去。

```cpp
struct REST_REQUEST {
bool (*AddHeader)(REST_REQUEST* request, char* key, char* val);
bool GetHeader
bool AddQueryParam
bool GetQueryParam
bool AddPathParam
bool GetPathParam
bool AddBody
bool GetBody
AddReqUri
GetReqUri
SetMethod
GetMethod
GetContentLength

private:
setEvhtpExchange
getEvhtpExchange
};

struct local_request{
 REST_REQUEST vtbl;
    int body_len;
    headerlen
querylen
pathparamlen
method
byte* body;
byte* uri;
map header
map queryparam;
map pathparam;
void * evhtp_exchange;
}

struct REST_RESPONSE {
AddHeader/GetHeader
SetStatusCode/GetStatusCode
AddBody/GetBody
GetContentLength
GetDownloadFile/SetDownloadFile
}

struct local_response{
 REST_response vtbl;
    int body_len;
    headerlen
status_code;
byte* body;
map header
}

method: get post put delete

// 外部方法接口
struct REST_RESOURCE
{
REST_HANDLE_REQUEST_FUNC get/post/put/delete;
};
int (*REST_HANDLE_REQUEST_FUNC)(REST_RESOURCE* source, REST_REQUEST* req, REST_RESPONSE* response)

//外部接口
struct server_interface {
int (*init)(server_interface* self, config);
int (*startServer)(server_interface* self);
int (*Addrouter)(server_interface* self, char* url, RESET_SOURCE* source);
Addglobalrouter/addchunkrouter
int (*asynRestNotifyFunc)(REST_REQUEST,REST_RESPONSE);
};

//内部实现
class server : public server_interface {
public:
static int init();
static int Addrouter(server_interface* self, char* url, RESET_SOURCE* source);
static int startServer
static int asynRestNotifyFunc
...
...
private:
void handle(evhtp_request_t req, void* resource);
void run();
void listen();

evhtp_t* evhtp;
evbase*;
map<string,REST_RESOURCE> router/globalrouter/chunkrouter;
pthread_t tid;// 后台线程运行evhtp
int pipe;// 监听服务器停止
}

server::Addrouter(server_interface* self, char* url, RESET_SOURCE* source){
    evhtp_set_cb(evhtp,url,server::Handle,source);// 对于从url来的请求，回调Handle函数，携带的参数就是source，包含了用户定义的处理接口函数
}

server::Handle(evhtp_request_t req, void* resource){
    //1. 根据evhtp的req创建REST_REQUEST，创建空的response
    REST_REQUEST *rreq = create(req); // 只包含让用户需要知道的信息相关的接口，比如uri、header、body等
    REST_RESPONSE *rep = create_rep(); // 用户需要回复的信息的接口
    REST_SOURCE* source = resource;
    // 2. 根据REST_SOURCE调对应的设置response回调  REST_SOURCE为用户设置的回调函数  最后发送response到客户端
    switch req->method:
        case post: rtn= source->putfunc(source, rreq, rep);
        case put:
        case delete:
        case get:
    if(rtn==sync){
        evhtp_send_reply(rreq.getbody(),status);
        destroy(rreq, rep);
    } else if(rtn==async){
        evhtp_set_pause();
        return ok;
    }
}

void listen(){
    // 创建线程，线程中运行Run函数
    pthread_create(&tid,Run);
    pthread_join(tid);
}

void Run(){
    evbaseloop(); // 阻塞
}

void startServer(){
    while(1)Liten(); //阻塞
}

// 异步调用，用户自己保存request和response，然后处理完后自己调这个函数异步发送
int asynRestNotifyFunc(REST_REQUEST req,REST_RESPONSE rep){
    req.getevhtpexchange();
    evthr_defer(thread,asynhandle,{req,rep});
    
}
// 内部函数
asynhandle(){
    evhtp_send_reply(get_body(rep), status);
    ev_request_resume(exchange);
}


// 客户端用法

handlerMap[] = {
  {"/V1/business1/resource1",
        { {METHOD_POST,handler1},{METHOD_GET,handler1_1} }
    },
  {"/V1/business1/resource2",
        { {METHOD_POST,handler2} }
    },
}

register(){
    RESOURCE* res;
    res->post = handler1;
    res->get = handler1_1;
    add_router("/V1/business1/resource1",res);
    RESOURCE* res2;
    res2->post = handler2;
    add_router("/V1/business1/resource2",res2);
}
```



