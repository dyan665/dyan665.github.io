# unique ptr

[返回主页](../../README.md)

# 原理
> 禁用拷贝构造、拷贝函数。

```cpp

template<class T>
class UniquePtr {
    public:
        UniquePtr():ptr(nullptr){}      
        UniquePtr(T* p):ptr(p){
            cout << "this is pointer func" << endl;
        }
        ~UniquePtr(){
            if(ptr)delete ptr;
            ptr = nullptr;
            cout << "this is pointer del func" << endl;
        }
        UniquePtr(const UniquePtr &p)=delete;//copy constructor
        UniquePtr(UniquePtr &&p) {//move constructor
            ptr = p.ptr;
            p.ptr= nullptr;
        }
        
        
    private:
        T *ptr;
};

template<class T>
class UniquePtr<T[]> {
    public:
        UniquePtr(T[]){
            cout << "this is pointer array func" << endl;
        }
};


//template<class T>
//void func(T t){
//  cout << t << endl;
//}


int main4(void){
    UniquePtr<int> test1(new int(2));
    UniquePtr<int> test11(move(test1));
    
    UniquePtr<int[]> test2(new int[2]{1,2});
    int test3[] = {3,4};
    int * a = test3;
    UniquePtr<int[]> test4(test3);
    UniquePtr<int[]> test5(a);
    
//  func(1212);
//  func("string12");
    
    return 0;
}

```
