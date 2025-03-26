# 模板函数的条件重载与特化

# 原理
> 1. 模板函数条件重载：一般基于SFINAE，即推导失败就放弃对应模板实例化，常用的方式就是通过enable_if控制条件；  
> 2. 模板函数特化：基于偏特化或者全特化，优先匹配最特化的版本；进阶版用法就是匿名模板参数下的标签分发；

```cpp

1. 条件重载
模板函数条件重载：注意，参数需要为T（serialize(string("aa"))调用，需要通过参数类型推导出T的类型），否则T无法推导可能导致enable_if失败。
// 处理 string 类型
template<typename T>
typename enable_if<is_same<T, string>::value, void>::type
serialize(const T &value, vector<char> &buffer) {
    // 字符串序列化逻辑
}

// 处理非字符串类型
template<typename T>
typename enable_if<!is_same<T, string>::value, void>::type
serialize(const T &value, vector<char> &buffer) {
    // 通用序列化逻辑
}

template<typename T>
typename enable_if<is_same<T, string>::value, void>::type
serialize(const string &value, vector<char> &buffer){}
当调用serialize(string("aa"))时，无法推导T，导致此模板失效！！！因为需要显式指定T，无法通过参数类型推导出T的类型。


2. 模板特化
模板函数特化：注意，必须要有一个基础版本，否则无法特化
// 原始模板（无 enable_if）
template<typename T>
void serialize(const T &value, vector<char> &buffer);

// 其他类型的通用实现
template<typename T>
void serialize(const T &value, vector<char> &buffer) {
    // 通用逻辑
}

// 显式特化 string 类型
template<>
void serialize<string>(const string &value, vector<char> &buffer) {
    // 字符串序列化逻辑
}


最好别两者混用，容易出问题

3. 进阶特化 标签分发
基于模板特化，若想对一类类型进行相同处理，比如数字类型（通过std::is_arithmetic<T>::value判定），可以将其判定作为模板参数传递，可以是bool或者标签类型都行。
#include <type_traits>
#include <vector>
using namespace std;
// 辅助类模板，默认处理非算术类型
template <typename T, bool IsArithmetic = false>
struct Serializer {
    static void serialize(const T& value, vector<char>& buffer) {
        // 通用类型的序列化逻辑
    }
};
// 部分特化：处理所有算术类型（通过第二个模板参数约束）
template <typename T>
struct Serializer<T, true> {  // 当 IsArithmetic=true 时触发
    static void serialize(const T& value, vector<char>& buffer) {
        // 算术类型的统一序列化逻辑
        const char* data = reinterpret_cast<const char*>(&value);
        buffer.insert(buffer.end(), data, data + sizeof(T));
    }
};
// 特化处理所有容器（示例）
template <typename Container>
struct Serializer<Container, false> {  // 假设通过特征检查是否为容器
    static void serialize(const Container& c, vector<char>& buffer) {
        for (const auto& item : c) {
            serialize(item, buffer); // 递归序列化元素
        }
    }
};


template <typename T>
void serialize(const T& value, vector<char>& buffer) {
    // 通过 std::is_arithmetic 判断是否为算术类型，传递给辅助类
    Serializer<T, std::is_arithmetic<T>::value>::serialize(value, buffer);
}
// 显式特化 std::string
template <>
void serialize<string>(const string& value, vector<char>& buffer) {
    buffer.insert(buffer.end(), value.begin(), value.end());
    buffer.push_back('\0'); // 添加终止符
}


4. 条件重载另一写法：匿名模板参数
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
void serialize(const T& value, std::vector<char>& buffer) {
    // 处理算术类型
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, std::string>>>
void serialize(const T& value, std::vector<char>& buffer) {
    // 处理字符串类型
}

注意：不能写通用版本，因为会导致二义性，匹配上多个模板，只能同时也写上条件，并且三者条件不能存在重合
比如合法的写法就是typename = std::enable_if_t<!std::is_same_v<T, std::string> && !std::is_arithmetic_v<T>>
如：serialize(3.2) 会匹配通用版本和浮点版本，存在二义性
template <typename T>
void serialize(const T& value, std::vector<char>& buffer) { // 禁止与上述两个同时存在
    // 通用版本实现
}

```

[返回主页](../../README.md)
