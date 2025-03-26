# 函数类型与函数指针间的转换


# 原理
> 1. 在针对函数类型或者函数指针类型上的推导，对于参数为引用，函数类型会保留，不会退化为函数指针，并且在函数类型下，对其const限定失效；无法将函数指针按引用的方式传递给函数类型引用；   
> 2. 在针对函数类型或者函数指针类型上的推导，对于参数为值传递，函数类型会退化为函数指针；   
> 3. 若参数为函数指针引用，传递的参数是函数名（即函数类型），此时函数类型隐式转换为函数指针，为右值，此时需要将函数指针引用声明为const，否则无法绑定到右值上；   



```cpp
1. 参数为引用方式下的推导

无法将函数指针按引用的方式传递给函数类型参数，两者不能隐式转换
可以在值传递的方式上，将函数类型隐式转换为函数指针

template<typename Func>
void testConst(const Func& func)   // 只有Func推导为函数指针，const才生效为const指针 类似void (*const)(int,int)，当Func推导为函数类型，const失效
void testNonconst(Func& func)
void test(Func func)

std::string add_func(std::string a, std::string b);

add_func 的类型是 函数类型：std::string(std::string, std::string)。
&add_func 的类型是 函数指针类型：std::string(*)(std::string, std::string)。
当函数名 add_func 作为参数传递时，默认会退化为函数指针（除非绑定到引用）。

testConst(add_func);
testNonconst(add_func);
函数名 add_func 会尝试绑定到引用。
由于引用可以绑定到函数类型（无需退化到指针），Func被推导为函数类型本身：std::string(std::string, std::string)。
参数 func 的实际类型是const Func&，但由于函数类型不能有const限定符，const会被忽略。最终func的类型为函数引用：std::string(&)(std::string, std::string)。

    
2. 参数为值传递下的推导
test(add_func);
func的类型：std::string(*)(std::string, std::string)。

函数名退化为指针的条件是：函数名被绑定到非引用参数或指针类型的参数。

3. 其它
void testConst(const Func& func) 
当显式传递模板参数，Func为函数指针时，即使传参是函数类型，也会被隐式转换为函数指针，此处必须为const，因为隐式转换出的是右值，只能绑定于const左引用。
testConst<string(*)(string,string)>(add_func);
add_func为函数类型，但Func是函数指针引用，因此将add_func隐式转换为函数指针。


```

[返回主页](../../README.md)
