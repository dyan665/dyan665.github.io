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

// 特化版本，适用于数组类型
template <typename T>
class SharedPtr<T[]> {
private:
    T* ptr;
    size_t* count;

    // 删除器：使用 delete[] 释放资源
    void deleter() {
        delete[] ptr;
    }

public:
    explicit SharedPtr(T* p = nullptr) : ptr(p), count(new size_t(1)) {
        if (p == nullptr) {
            *count = 0;
        }
    }

    // 拷贝构造
    SharedPtr(const SharedPtr<T[]>& other) : ptr(other.ptr), count(other.count) {
        if (ptr) {
            (*count)++;
        }
    }

    ~SharedPtr() {
        release();
    }

    // 拷贝赋值
    SharedPtr<T[]>& operator=(const SharedPtr<T[]>& other) {
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

    T& operator[](size_t index) const {
        return ptr[index];
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
            deleter();  // 调用删除器
            delete count;
        }
    }
};

int main() {
    // 测试非数组类型
    SharedPtr<int> p1(new int(10));
    std::cout << "p1 use_count: " << p1.use_count() << std::endl; // 1
    std::cout << "p1 value: " << *p1 << std::endl; // 10

    // 测试数组类型
    SharedPtr<int[]> p2(new int[5]{1, 2, 3, 4, 5});
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 1
    std::cout << "p2[2]: " << p2[2] << std::endl; // 3

    // 测试拷贝构造
    SharedPtr<int[]> p3 = p2;
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 2
    std::cout << "p3 use_count: " << p3.use_count() << std::endl; // 2

    // 测试重置
    p2.reset();
    std::cout << "p2 use_count: " << p2.use_count() << std::endl; // 0
    std::cout << "p3 use_count: " << p3.use_count() << std::endl; // 1

    return 0;
}

```
