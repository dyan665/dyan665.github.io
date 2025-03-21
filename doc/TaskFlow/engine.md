# 基于taskflow的通用引擎

[返回主页](../../README.md)

# 原理
> 核心机制就是反射与taskflow，将任务整体拆分为一个有向无环图，每个节点为一个子任务，在json配置文件中定义有向无环图，利用反射实例化子任务节点为函数对象，父任务节点的输出为子任务节点的输入，利用taskflow并行执行，提高并发能力降低延迟。
> taskflow会将入度为0的节点并行执行，当节点的依赖项执行完毕，就会自动将节点放入线程池开启运行。

## 配置文件
> pipeline：子任务数组，子任务中name为子任务的唯一名称；operation代表对应的需要实例化的操作函数对象；bottom表示依赖的节点，bottom中包含的节点的输出为当前节点的输入，整体pipeline构成子任务有向无环图；model为该操作对象依赖的模型；param为对应模型所需的参数。
```json
{
    "appName": "IDCard1",
    "appType": "IDCard",
    "pipeline": [
        {
            "name": "rectify_0",
            "operation": "rectify",
            "bottom": [
                ""
            ],
            "params": {
                "p1": 1,
                "valid": true
            },
            "model": [
                "rec_model_2"
            ]
        },
        {
            "name": "rectify_1",
            "operation": "rectify",
            "bottom": [
                ""
            ],
            "params": {
                "p1": 3,
                "valid": true
            },
            "model": [
                "rec_model_2"
            ]
        },
        {
            "name": "obj_detect_0",
            "operation": "obj_detect",
            "bottom": [
                "rectify_0",
                "rectify_1"
            ],
            "params": {
                "p2": 2
            },
            "model": [
                "obj_detect_model_1"
            ]
        }
    ],
    "models": [
        {
            "rec_model_1": "./rec_model_simple"
        },
        {
            "rec_model_2": "./rec_model_complex"
        },
        {
            "obj_detect_model_1": "./obj_detect_model_2025.1.1"
        }
    ]
}
```
## 核心代码

### 反射工厂
```cpp
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <utility>
#include <type_traits>
#include <typeinfo>

using namespace std;

template <typename T>
struct CallableTraits;

// 特化普通函数指针
template <typename Ret, typename... Args>
struct CallableTraits<Ret(*)(Args...)> {
    using ReturnType = Ret;
    using ArgsTuple = tuple<Args...>;
};

// 特化lambda或函数对象中函数指针
template <typename R, typename className, typename... Args> //<typename Functor>
struct CallableTraits<R(className::*)(Args...)const> {
    using ReturnType = R;
    using ArgsTuple = tuple<Args...>;
};

// 特化lambda或函数对象
template <typename Functor>
struct CallableTraits {
    private:
        using CallType = decltype(&Functor::operator());
    public:
        using ReturnType = typename CallableTraits<CallType>::ReturnType;
        using ArgsTuple = typename CallableTraits<CallType>::ArgsTuple;
};

// 特化function
template <typename Ret, typename... Args>
struct CallableTraits<function<Ret(Args...)>> {
    using ReturnType = Ret;
    using ArgsTuple = tuple<Args...>;
};

template <typename T, typename V>
struct FunctionTypeHelper;

template <typename Ret, typename... Args>
struct FunctionTypeHelper<Ret, tuple<Args...>> {
    using Type = function<Ret(Args...)>;
};

// 将任意可调用对象转换为function
template <typename Functor>
auto ToFunction(Functor&& func) {
    using Traits = CallableTraits<decay_t<Functor>>;
    using ReturnType = typename Traits::ReturnType;
    using ArgsTuple = typename Traits::ArgsTuple;

    using FunctionType = typename FunctionTypeHelper<ReturnType, ArgsTuple>::Type;
    return FunctionType(forward<Functor>(func));
}

template <typename T>
class Factory {
    public:
        // 定义Creator，new T
        template <typename... Args>
        using Creator = function<T*(Args...)>; // 函数签名忽略顶层const，注意强制转换

        // 添加Creator到注册表
        template <typename Functor>
        static void addCreator(const string &type, Functor&& func) {
            using FunctionType = decltype(ToFunction(forward<Functor>(func)));
            auto wrapper = make_shared<FunctionType>(ToFunction(forward<Functor>(func)));
            Registry()[type] = wrapper;
        }

        // 创建接口
        template <typename... Args>
        static T *create(const string &type, Args&&... args) {
            auto it = Registry().find(type);
            if (it == Registry().end()) {
                cerr << "Unknown type: " << type << endl;
                return nullptr;
            }

            // 将shared_ptr<void>转换回具体的Creator类型
            using CreatorType = Creator<Args...>;
            auto creatorWrapper = static_pointer_cast<CreatorType>(it->second);
            if (!creatorWrapper) {
                cerr << "Type mismatch for: " << type << endl;
                return nullptr;
            }

            // 调用Creator创建T对象
            return (*creatorWrapper)(forward<Args>(args)...);
        }

    private:
        // 注册表
        using CreatorRegistry = unordered_map<string, shared_ptr<void>>;

        static CreatorRegistry &Registry() {
            static CreatorRegistry registry;
            return registry;
        }

        Factory() = default;
};


template <typename T>
class Registerer {
    public:
        template<typename Creator>
        Registerer(const string &type, Creator&& creator) {
            Factory<T>::addCreator(type, forward<Creator>(creator));
        }
};


#define REGISTER_OP_CLASS(AppName, OpName)                                   \
static IOperator *Creator_##AppName##_##OpName()                             \
{									     \
	return new AppName::OpName();                                        \
}                                                                            \
Registerer<IOperator> g_creator_##AppName##_##OpName{ #AppName "." #OpName,  \
	Creator_##AppName##_##OpName };


#define REGISTER_APP_CLASS(type)                                             \
static BaseAPP *Creator_##type()                                             \
{                                                                            \
	return new type();                                                   \
}                                                                            \
Registerer<BaseAPP> g_creator_##type{ #type, Creator_##type };

class IOperator;
class BaseAPP;

template class Factory<IOperator>;
template class Factory<BaseAPP>;

template class Registerer<IOperator>;
template class Registerer<BaseAPP>;


// operator interface
class IOperator {
    public:
        virtual void setConf(void *config) = 0;
        virtual void run(void *inputJson, void *outputJson) = 0;
};

class BaseAPP {
    public:
        // Rectofy base版本，可被其它未定义同名op的其它app共用
        class OperatorRectofy: public IOperator {
            public:
                void setConf(void *config) override {
                    cout << "set BaseAPP::OperatorRectofy config" << endl;
                }
                void run(void *inputJson, void *outputJson) override {
                    cout << "this is BaseAPP::OperatorRectofy operation" << endl;
                }
        };
        // class OtherOperation: public IOperator

        void create(const unordered_map<string, string> &config) {//void *json
            //根据配置进行创建taskflow
            string opname = config.at("appName") + "." + config.at("operation"); // appname与opname拼接，参数均从配置文件得到
            //运行taskfow
            auto opPtr = Factory<IOperator>::create(opname);
            ops.emplace_back(opPtr);
        }
        void run() {
            for (const auto& op : ops) {
                //set model param config in operation
                op->setConf(nullptr);
            }
            //set inputs and outputs, then run taskflow
            for (const auto& op : ops) {//基于taskflow并行运行，此处模拟
                op->run(nullptr, nullptr);
            }
        }
    protected:
        vector<shared_ptr<IOperator>> ops;
        // vector<json> inputs;
        // vector<json> outputs;

};

class IDCard : public BaseAPP {
    public:
        // 专属 IDCard app的 Rectofy
        class OperatorRectofy: public IOperator {
            public:
                void setConf(void *config) override {
                    cout << "set IDCard::OperatorRectofy config" << endl;
                }
                void run(void *inputJson, void *outputJson) override {
                    cout << "this is IDCard::OperatorRectofy operation" << endl;
                }
        };
        // class OtherOperation: public IOperator
};

class License : public BaseAPP {
    public:
        // 专属 License app的 Rectofy
        class OperatorRectofy: public IOperator {
            public:
                void setConf(void *config) override {
                    cout << "set License::OperatorRectofy config" << endl;
                }
                void run(void *inputJson, void *outputJson) override {
                    cout << "this is License::OperatorRectofy operation" << endl;
                }
        };
        // class OtherOperation: public IOperator
};

REGISTER_APP_CLASS(BaseAPP);
REGISTER_OP_CLASS(BaseAPP, OperatorRectofy);

REGISTER_APP_CLASS(IDCard);
REGISTER_OP_CLASS(IDCard, OperatorRectofy);

REGISTER_APP_CLASS(License);
REGISTER_OP_CLASS(License, OperatorRectofy);

int main() {
    // config read from file
    unordered_map<string, string> config = {
        {"appName", "IDCard"},
        {"operation", "OperatorRectofy"}
    };
    auto app = Factory<BaseAPP>::create(config.at("appName"));
    app->create(config);
    app->run();

}
```































