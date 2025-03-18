# 完美转发

[返回主页](../../README.md)

# 原理
> 在模板函数传递参数时，保持参数的左右值属性，核心就是remove_reference针对左右值类型的萃取，以及引用折叠；
> 引用折叠：&& & -> &；&& && -> &&；


```cpp
namespace yy {
    // remove reference
    template<class T>
    struct remove_reference{
        typedef T type;
    };
    template<class T>
    struct remove_reference<T&>{
        typedef T type;
    };
    template<class T>
    struct remove_reference<T&&>{
        typedef T type;
    };
    
    // move
    template<class T>
    typename yy::remove_reference<T>::type&& move(T&& t){
        return static_cast<typename yy::remove_reference<T>::type&&>(t);
    }
    
    // forward //普通重载模板，而不是万能引用（参数直接为type&、type&&），因此调用的时候，根据参数类型调左右值不同版本，
    //对于参数type，则为非依赖类型，无法从参数类型中推导出T的类型，需要指明模板参数T的类型，从而正确推导
    // forward<T>(xxx) 返回值类型取决于forward中的T类型，int、 int&、 int&&  纯右值为非引用，具名变量左值，move(x)为右值
    template<class T>
    T&& forward(typename yy::remove_reference<T>::type& t) {// perfect forward for lvalue   左值引用类型
        cout << "T&" <<endl;
        return static_cast<T&&>(t);//此处与返回值均存在引用折叠
    }
    template<class T>
    T&& forward(typename yy::remove_reference<T>::type&& t) {// perfect forward for rvalue  右值引用类型
        cout << "T&&" <<endl;
        return static_cast<T&&>(t);//此处与返回值均存在引用折叠
    }
}

void foo(int &x)
{
    std::cout << "lvalue: " << x << std::endl;
}

void foo(int &&x)
{
    std::cout << "rvalue: " << x << std::endl;
}

template <typename T>
void bar(T &&x)
{
    foo(yy::forward<T>(x));
}


int main(void) {
    //int a = 1;
    //int&& b = 1;
    
    //yy::forward<decltype(a)>(a); // T&
    //yy::forward<decltype(a)>(b); // T&
    //yy::forward<decltype(b)>(b); // T& decltype(b)=int&& 但参数b是左值，因此调左值引用类型forward，返回的是T&&右值
    //yy::forward<decltype(yy::move(a))>(yy::move(a)); // T&&

    // another test
    int a = 1;
    int &b = a;
    int &&c = 2;

    bar(123); // 123推导bar中T为int，但bar中x为左值，因此调左值引用类型forward，返回T&&也就是int&& foo对应就是右值版本
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int
    // rvalue: 123

    bar(a);
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 1

    bar(b); // b为具名左值
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 1

    bar(c); // c为具名左值
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 2

    bar(std::move(a)); // move(a)为右值 调右值版本的foo，但x还是左值 具名右值引用x为左值
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int
    // rvalue: 1

    return 0;
}

```
