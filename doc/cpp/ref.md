# 万能引用

[返回主页](../../README.md)

# 原理
> 引用折叠，forward完美转发，不改变参数的左右值属性；

```cpp
// 万能引用条件  1.类型T必须在函数参数中推导；2.参数类型为T&&
// 非万能引用  模板类中成员函数T&&   参数类型为非依赖类型
template <typename T>
void bar(T&& x) {
    foo(std::forward<T>(x));
}

// 右值引用，T在类实例化时已经被推导出
template <typename T>
class MyClass {
public:
    void foo(T&& arg);
};

// 万能引用 U在参数时推导出
template <typename T>
class MyClass {
public:
    template <typename U>
    void foo(U&& arg) {
        // 使用 std::forward 实现完美转发
        bar(std::forward<U>(arg));
    }
};

// 非依赖参数，非万能引用
template <typename T>
void bar(int&& x) {
    foo(std::forward<T>(x));
}

// 非依赖参数，非万能引用，虽然T需要推导，但参数类型固定为右值引用
template <typename T>
void foo(typename std::remove_reference<T>::type&& arg) {
    std::cout << "Called foo with rvalue reference" << std::endl;
}

int main() {
    int x = 10;
    // foo(x); // 错误：不能将左值绑定到右值引用 对应T为int&，但参数为int&&
    foo<int>(10); // 正确：10 是右值，可以绑定到 int&&
    foo<int&&>(std::move(x)); // 正确：std::move(x) 是右值，可以绑定到 int&&
    return 0;
}

```
