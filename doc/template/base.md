# 基础类型推导

调用f(expr)，根据expr的类型推导出T和param的类型，推导规则取决于param的类型声明形式。
# 1.基本模板类型推导规则
## 1.1 param是值类型（pass-by-value）
    template<typename T>
    void f(T param);
    如果expr是int，则T推导为int，param的类型也是int。
    如果expr是const int，则T推导为int，param的类型也是int（const被忽略）。
    如果expr是int&，则T推导为int，param的类型是int（引用被忽略）。
    如果expr是int&&，则T推导为int，param的类型是int（引用被忽略）。
    如果expr是const int&，则T推导为int，param的类型是int（const和引用都被忽略）。
    
## 1.2 param是引用类型（pass-by-reference）
    template<typename T>
    void f(T& param);
    如果expr是int，则T推导为int，param的类型是int&。
    如果expr是const int，则T推导为const int，param的类型是const int&。
    如果expr是int&，则T推导为int，param的类型是int&。
    如果expr是int&&，则推导错误，右值无法绑定左值引用。
    如果expr是const int&，则T推导为const int，param的类型是const int&。
    
## 1.3 param是常量引用类型（pass-by-const-reference）
    template<typename T>
    void f(const T& param);
    如果expr是int，则T推导为int，param的类型是const int&。
    如果expr是const int，则T推导为int，param的类型是const int&。
    如果expr是int&，则T推导为int，param的类型是const int&。
    如果expr是int&&，则T推导为int，param的类型是const int&，右值能绑定到常量左值引用。
    如果expr是const int&，则T推导为int，param的类型是const int&。
    
## 1.4 万能引用
    template<typename T>
    void f(T&& param);
    template<typename... Args>
    void f(Args&&... args);
    核心区分左值与右值，左值推导T为对应类型的引用，右值推导T为对应类型，保留顶层const，比如：
    f(10) // 右值 T为int param为int&&
    f(string("s")) // 右值 T为string param为string&&
    string s;f(s) // 左值 T为string& param为string&
    const string s;f(s) //左值 T为const string& param为const string&
    const string* const s;f(s) // 左值 T为const string* const&，param为const string* const
    int&& a =1;f(1) // 注意，右值引用值类型为左值 类型是右值引用 传递的是左值 因此T为int&，param为int&，绑定在左值引用上的是a这个左值
    
## 1.5 const指针
    template<typename T>
    void f(const T* param);
    如果expr是int*，则T推导为int，param的类型是const int*。
    如果expr是const int*，则T推导为int，param的类型是const int*。
    如果expr是int**，T推导为 int*，param的类型是 const int**。（对于多级指针，按照指针的层级逐层匹配即可）


# 2. 指针与数组
    如果expr是数组类型（如int arr[10]），则T推导为int*，param的类型是int*（数组退化为指针）。
    如果expr是函数类型（如void func(int)），则T推导为函数指针类型（如void (*)(int)），param的类型是函数指针。

# 3. 临时对象
    按右值推导即可
    
# 4. enum
    按类推导即可
    
# 5. 字面量
    比如14、14.0、临时对象，"this is a msg"字符串字面量较为特殊，普通推导退化为const char*指针，引用推导为const char(&)[x]，其它作为右值进行推导。
    

对于 expr 是 const int * const 的情况：
| 模板参数声明形式 | T 推导类型       | param 类型           |
|:----------------:|:----------------:|:--------------------:|
| T param          | const int*       | const int*           |
| T& param         | const int* const | const int* const&    |
| T* param         | const int        | const int*           |
| const T* param   | int              | const int*           |
| const T& param   | const int*       | const int* const&    |
  


# 总结
    值类型：顶层const和引用会被忽略，推导出的是去掉顶层const和引用后的类型。
    引用类型：顶层const会被保留，推导出的是带有顶层const和引用的类型。
    复杂类型：如struct、指针、数组、函数等，推导规则与基本类型类似，但需要考虑类型本身的复杂性。
    指针传递会忽略指针本身的 const，但保留指向对象的 const，也就是保留底层const，去除顶层const。
    普通模板类型推导不会将 T 推导为引用类型，引用会被忽略，也就是值类型推导。


    T可以被推导为引用类型的情况：
        使用转发引用（T&&），此时 T 可以根据实参的左右值属性推导为引用类型，当传递一个左值，T被推导为引用如int&，当传递一个右值（如10）时，T被推导为类型本身如int。
        显式指定 T 为引用类型（如 f<int&>(ref)）。
    如果希望 T 本身是引用类型，必须使用转发引用或显式指定。


[返回主页](../../README.md)
