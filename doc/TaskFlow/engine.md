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
template <typename T, typename... Args> class Factory
{
public:
    using Creator = std::function<T *(Args...)>;
    using CreatorRegistry = std::unordered_map<std::string, Creator>;
    static CreatorRegistry &Registry();
    // Add a creator.
    static void AddCreator(const std::string &type, Creator creator);
    // Get an operator using a type.
    static T *Create(const std::string &type, Args &&... args);
    static bool HasEntry(const std::string &type);
private:
    Factory() = default; 
};

template <typename T, typename... Args> class Registerer
{
public:
    using Creator = T *(*)(Args...);
    Registerer(const std::string &type, Creator creator);
};

#define REGISTER_OP_CLASS(AppName, OpName)                                         \
static IOperator *Creator_##AppName##_##OpName()                                   \
{                                                                                  \
return new AppName::OpName();                                                      \
}                                                                                  \
Registerer<IOperator> g_creator_##AppName##_##OpName{ #AppName "." #OpName,        \
Creator_##AppName##_##OpName };

#define REGISTER_APP_CLASS(type)                                                  \
static BaseAPP *Creator_##type()                                                  \
{                                                                                 \
return new type();                                                                \
}                                                                                 \
Registerer<BaseAPP> g_creator_##type{ #type, Creator_##type };


template <typename T, typename... Args>
typename Factory<T, Args...>::CreatorRegistry &Factory<T, Args...>::Registry()
{
    static CreatorRegistry g_registry_;
    return g_registry_;
}

// Add a creator.
template <typename T, typename... Args>
void Factory<T, Args...>::AddCreator(const std::string &type, Creator creator)
{
    CreatorRegistry &registry = Registry();
    if (registry.count(type))
        std::cout << type << " already registered." << std::endl;
    registry[type] = creator;
}

// Get an operator using a type.
template <typename T, typename... Args>
T *Factory<T, Args...>::Create(const std::string &type, Args &&... args)
{
    CreatorRegistry &registry = Registry();
    if (!HasEntry(type)) {
        std::cout << "Unknown type: "<< type << std::endl;
        return nullptr;
    }
    return registry[type](std::forward<Args>(args)...);
}

template <typename T, typename... Args>
bool Factory<T, Args...>::HasEntry(const std::string &type)
{
    return Registry().count(type);
}

template <typename T, typename... Args>
Registerer<T, Args...>::Registerer(const std::string &type, Creator creator)
{
    Factory<T, Args...>::AddCreator(type, creator);
}

template class Factory<Operator>;
template class Factory<BaseAPP>;

template class Registerer<Operator>;
template class Registerer<BaseAPP>;
```

### 






























