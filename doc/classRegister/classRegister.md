# 类反射工厂

[返回主页](../../README.md)

## 原理
> 单例工厂类`Factory`，存储类名和创建函数对象的映射，提供注册与创建接口，注册记录映射，创建则通过函数对象创建对应的类对象；
> 注册辅助类`Register`，构造函数中调工厂的注册接口注册，主要作用是通过定义全局变量的方式（一般定义为一个宏），通过其构造函数自动进行注册；


```cpp

/* part 1: single + register  */

class Factory {
    public:
        static Factory& get(){
            static Factory f;
            return f;
        }
        static void fregister(string key,function<int*()> f){
            get()._map[key] = f;
        }
        static int* create(string key){
            if(get()._map.count(key))
            return get()._map[key]();
            cout<<"key:"<<key<<" not exist."<<endl;
            return nullptr;
        }
    
    private:
        unordered_map<string,function<int*()>> _map;
        Factory()=default;
        Factory(const &Factory) = delete;
        Factory& operator=(const Factory&) = delete;
        ~Factory()=default;
};

class Register{
    public:
        Register(string key,function<int*()> f){
            Factory::fregister(key,f);
        }
};

int * test(){
    return nullptr;
}

int * test2(){
    return new int(3);
}

Register plugin("test",test);
Register plugin2("test2",test2);

//#define main1 main

int main1(void){
    auto t = Factory::create("test2");
    if(!t)cout<<"t is null"<<endl;
    else cout<<"t is "<<*t<<endl;
    return 0;   
}

/* TODO more classical by template */

```
