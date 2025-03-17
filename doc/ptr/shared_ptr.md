# shared ptr

[返回主页](../../README.md)

# 原理
> 引用计数为0释放资源，重载拷贝构造、拷贝赋值。

```cpp
#include <iostream>

template <typename T>
class SharedPtr {
private:
    T* ptr;          // 指向管理的资源
    size_t* count;   // 引用计数

public:
    explicit SharedPtr(T* p = nullptr) : ptr(p), count(new size_t(1)) {
        if (p == nullptr) {
            *count = 0;
        }
    }

    // 拷贝构造
    SharedPtr(const SharedPtr<T>& other) : ptr(other.ptr), count(other.count) {
        if (ptr) {
            (*count)++;
        }
    }

    ~SharedPtr() {
        release();
    }

    // 拷贝赋值
    SharedPtr<T>& operator=(const SharedPtr<T>& other) {
        if (this != &other) {
            release();
            ptr = other.ptr;
            count = other.count;
            if (ptr) {
                (*count)++;
            }
        }
        return *this;
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

    size_t use_count() const {
        return *count;
    }

    T* get() const {
        return ptr;
    }

    void reset(T* p = nullptr) {
        release();
        ptr = p;
        count = new size_t(1);
        if (p == nullptr) {
            *count = 0;
        }
    }

private:
    void release() {
        if (ptr && --(*count) == 0) {
            delete ptr;
            delete count;
        }
    }
};

int main() {
    SharedPtr<int> p1(new int(10));
    std::cout << "p1 use_count: " << p1.use_count() << std::endl; // 1

    SharedPtr<int> p2 = p1;
    std::cout << "p1 use_count: " << p1.use_count() << std::endl; // 2
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 2

    SharedPtr<int> p3(new int(20));
    p3 = p2;
    std::cout << "p1 use_count: " << p1.use_count() << std::endl; // 3
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 3
    std::cout << "p3 use_count: " << p3.use_count() << std::endl; // 3

    p1.reset();
    std::cout << "p1 use_count: " << p1.use_count() << std::endl; // 0
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 2
    std::cout << "p3 use_count: " << p3.use_count() << std::endl; // 2

    return 0;
}

```
