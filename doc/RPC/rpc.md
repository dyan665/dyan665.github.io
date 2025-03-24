# RPC C++实现

## 原理
> RPC分为序列化、网络传输、反序列化、函数映射与调用，其中主要为参数的序列化与反序列化较为麻烦。客户端序列化函数名称与参数类型，通过网络传输给服务端，服务端然后从缓冲区反序列化出对应的函数名称与参数，通过参数去调用对应的函数，最终序列化结果然后返回即可。

## C++实现
```cpp
#include <string>
#include <iostream>
#include <unordered_map>
#include <type_traits>
#include <vector>
#include <functional>
#include <tuple>
#include <cstring>
#include <mutex>
#include <future>
#include <algorithm>
#include <thread>
#include <condition_variable>

using namespace std;

// hook number convert function
uint16_t htons(uint16_t val) {
    return val;
}
uint32_t htonl(uint32_t val) {
    return val;
}
uint64_t htonll(uint64_t val) {
    return val;
}
uint16_t ntohs(uint16_t val) {
    return val;
}
uint32_t ntohl(uint32_t val) {
    return val;
}
uint64_t ntohll(uint64_t val) {
    return val;
}
uint64_t a_htonll(uint64_t value) {
    static const int num = 42;
    if (*reinterpret_cast<const char * >(&num) == num) { // 小端
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    } else {// 大端 网络序
        return value;
    }
}
uint64_t a_ntohll(uint64_t value) {
    return htonll(value);
}

// 通用序列化函数
template<typename T>
typename enable_if < !is_arithmetic<T>::value
&& !is_same<T, string>::value, void >::type serialize(const T &value, vector<char> &buffer) {
//	cout << typeid(T).name()<<endl;
    cout << "serialize default" << endl;
    const char* begin = reinterpret_cast<const char *>(&value);
    const char* end = begin + sizeof(value);
    buffer.insert(buffer.end(), begin, end);
}

// 特化number
template<typename T>
typename enable_if<is_arithmetic<T>::value, void>::type serialize(const T &value, vector<char> &buffer) {
//	cout << typeid(T).name()<<endl;
    cout << "serialize number" << endl;
    if  (sizeof(T) == sizeof(uint16_t)) {
        uint16_t netValue = htons(static_cast<uint16_t>(value));
        buffer.insert(buffer.end(), reinterpret_cast<char *>(&netValue),
                      reinterpret_cast<char *>(&netValue) + sizeof(netValue));
    } else if  (sizeof(T) == sizeof(uint32_t)) {
        uint32_t netValue = htonl(static_cast<uint32_t>(value));
        buffer.insert(buffer.end(), reinterpret_cast<char *>(&netValue),
                      reinterpret_cast<char *>(&netValue) + sizeof(netValue));
    } else if  (sizeof(T) == sizeof(uint64_t)) {
        uint64_t hostValue = static_cast<uint64_t>(value);
        uint64_t netValue = htonll(hostValue);
        buffer.insert(buffer.end(), reinterpret_cast<char *>(&netValue),
                      reinterpret_cast<char *>(&netValue) + sizeof(netValue));
    } else {
        // float、double等
        buffer.insert(buffer.end(), reinterpret_cast<const char *>(&value),
                      reinterpret_cast<const char *>(&value) + sizeof(value));
    }
}

// 特化string
void serialize(const string &value, vector<char> &buffer) {
//	cout << typeid(decltype(value)).name()<<endl;
    cout << "serialize string" << endl;
    // 写入字符串长度
    uint32_t length = htonl(static_cast<uint32_t>(value.size()));
    buffer.insert(buffer.end(), reinterpret_cast<char *>(&length), reinterpret_cast<char *>(&length) + sizeof(length));
    // 写入字符串内容
    buffer.insert(buffer.end(), value.begin(), value.end());
}

// 反序列化函数特化number
template<typename T>
typename enable_if<is_arithmetic<T>::value, T>::type
deserialize(const char *&buffer) {
    T value;
    memcpy(&value, buffer, sizeof(value));
    buffer += sizeof(value);
    if  (sizeof(T) == sizeof(uint16_t)) {
        value = ntohs(static_cast<uint16_t>(value));
    } else if  (sizeof(T) == sizeof(uint32_t)) {
        value = ntohl(static_cast<uint32_t>(value));
    } else if  (sizeof(T) == sizeof(uint64_t)) {
        value = ntohll(static_cast<uint64_t>(value));
    }
    return value;
}

// 反序列化函数特化string
template<typename T>
typename enable_if<is_same<T, string>::value, T>::type
deserialize(const char *&buffer) {
    // 读取字符串长度
    uint32_t length = ntohl(*reinterpret_cast<const uint32_t *>(buffer));
    buffer += sizeof(uint32_t);
    // 读取字符串内容
    string value(buffer, length);
    buffer += length;
    return value;
}

template <typename... Args, size_t... I>
tuple<Args...> deserialize_tuple_impl(const char *&data, index_sequence<I...>) {
    return tuple<Args...> {deserialize<tuple_element_t<I, tuple<Args...>>>(data)...};
}

// 反序列化为tuple主函数
template <typename... Args>
tuple<Args...> deserialize_tuple(const char *&data) {
    return deserialize_tuple_impl<Args...>(data, index_sequence_for<Args...> {});
}

class RpcServer {
    public:
        RpcServer() = default;
        void stop() {
            _stop = true;
            _cv.notify_all();
        }
        // 注册函数
        template<typename Func, typename... Args>
        void registerFunction(const string &name, Func&& func) {
            // 将函数包装为一个function，入参出参均为缓冲区，并带参数类型Args
            functions[name] = [func = forward<Func>(func), this](const vector<char> &buffer) -> vector<char> {
                return invokeFunction<Func, Args...>(func, buffer);
            };
        }
        // hook send func,客户端调用hook网络发送，以及服务端接收rpc请求
        void hookSend(const vector<char> &buffer, promise<vector<char>> result) {
            lock_guard<mutex> lck(_mu);
            requests.push_back(buffer);
            responses.push_back(move(result));
            _cv.notify_one();
        }
        void serve() {
            while (1) {
                unique_lock<mutex> lck(_mu);
                _cv.wait(lck, [&]() {
                    return !requests.empty() || _stop;
                });
                if (requests.empty())break;
                auto req = requests.back();
                // parse func name and params,then call and return result
                auto it = find(req.begin(), req.end(), '\0');
                if (it == req.end()) {
                    cout << "cant find 0 in req" << endl;
                } else {
                    string funcName(req.begin(), it);
                    cout << "func name:" << funcName << endl;
                    vector<char> params(it + 1, req.end());
                    for (auto t : params)cout << (int)t << " ";
                    cout << endl;
                    auto re = invoke(funcName, params);
                    responses.back().set_value(re);

                }
                requests.pop_back();
                responses.pop_back();
            }
        }
    private:

        vector<char> invoke(const string &functionName, const vector<char> &buffer) {
            auto it = functions.find(functionName);
            if (it != functions.end()) {
                return it->second(buffer);
            } else {
                throw runtime_error("Function not found");
            }
        }

        // 统一调用函数接口
        template<typename Func, typename... Args>
        vector<char> invokeFunction( Func const &func, const vector<char> &buffer) {
            // 从缓冲区反序列化参数为tuple
            const char* data = buffer.data();
            auto deserializedParams = deserializeArgs<Args...>(data);
            cout << "invokeFunction: " << get<0>(deserializedParams) << " " << get<1>(deserializedParams) << endl;
            // 调用函数并序列化返回值
            return serializeResult(func, deserializedParams);
        }


        // 从缓冲区反序列化为参数tuple
        template<typename... Args>
        tuple<Args...> deserializeArgs(const char *&data) {
            return deserialize_tuple<Args...>(data);
        }

        // 调用rpc函数并序列化返回值返回
        template<typename Func, typename... Args>
        vector<char> serializeResult(const Func &func, const tuple<Args...> &args) {
            auto result = callFunc(func, args, make_index_sequence<sizeof...(Args)> {});
            vector<char> buffer;
            serialize(result, buffer);
            return buffer;
        }

        template<typename Func, typename Tuple, size_t... I>
        auto callFunc(const Func &func, Tuple&& params, index_sequence<I...>) {
            return func(get<I>(forward<Tuple>(params))...);
        }

        // 保存rpc函数映射
        unordered_map<string, function<vector<char>(const vector<char>&)>> functions;
        vector<vector<char>> requests;
        vector<promise<vector<char>>> responses; // hook for sync socket request
        mutex _mu;
        condition_variable _cv;
        bool _stop = false;
};

// RPC client
class RpcInvoker {
    public:
        template<typename... Args>
        vector<char> invoke(const string &functionName, Args&&... args) {
            // 序列化函数名
            vector<char> buffer{functionName.begin(), functionName.end()};
            buffer.push_back('\0');
            // 序列化参数到缓冲区
            initializer_list<int> {(serialize(forward<Args>(args), buffer), 0)...};
            cout << "caller: ";
            for (auto t : buffer)cout << int(t) << " ";
            cout << endl;

            // send to server, wait for response
            promise<vector<char>> response;
            auto fu = response.get_future();
            server->hookSend(buffer, move(response));
            auto resp = fu.get();
            return resp;
        }
        void connectServer(RpcServer *serv) {
            server = serv;
        }
    private:
        RpcServer *server;
};

// rpc function
int add(int a, int b) {
    return a + b;
}

string concat(const string &a, const string &b) {
    cout << a << " " << b << endl;
    return a + b;
}

string concat2(const int &a, const string &b) {
    cout << a << " " << b << endl;
    return to_string(a) + "_concat2_" + b;
}

int main() {
    RpcServer server;
    RpcInvoker invoker;
    invoker.connectServer(&server); // hook for connection with rpc server
    thread backServ([&server] {server.serve();});

    // RPC server
    server.registerFunction<decltype(&add), int, int>("add", add);
    server.registerFunction<decltype(&concat), string, string>("concat", concat);
    server.registerFunction<decltype(&concat2), int, string>("concat2", concat2);

    // RPC client
    vector<char> result1 = invoker.invoke("add", 3, 4);
    {
        const char* t = result1.data();
        cout << "Result of add: " << deserialize<int>(t) << endl;
    }
    vector<char> result2 = invoker.invoke("concat", string("HA"), string("W"));
    {
        const char* t = result2.data();
        cout << "Result of concat: " << deserialize<string>(t) << endl;
    }
    vector<char> result3 = invoker.invoke("concat2", 123, string("W"));
    {
        const char* t = result3.data();
        cout << "Result of concat2: " << deserialize<string>(t) << endl;
    }
    server.stop();
    backServ.join();
    return 0;
}

//serialize number
//serialize number
//caller: 97 100 100 0 3 0 0 0 4 0 0 0
//func name:add
//3 0 0 0 4 0 0 0
//invokeFunction: 3 4
//serialize number
//Result of add: 7
//serialize string
//serialize string
//caller: 99 111 110 99 97 116 0 2 0 0 0 72 65 1 0 0 0 87
//func name:concat
//2 0 0 0 72 65 1 0 0 0 87
//invokeFunction: HA W
//HA W
//serialize string
//Result of concat: HAW
//serialize number
//serialize string
//caller: 99 111 110 99 97 116 50 0 123 0 0 0 1 0 0 0 87
//func name:concat2
//123 0 0 0 1 0 0 0 87
//invokeFunction: 123 W
//123 W
//serialize string
//Result of concat2: 123_concat2_W

```


[返回主页](../../README.md)

