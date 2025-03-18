# unique ptr

[返回主页](../../README.md)

# 原理
> 禁用拷贝构造、赋值，实现移动拷贝、移动赋值，实现解引用，偏特化数组版本，unique_ptr管理数组生命周期，其余访问全和数组一致。

```cpp

#include <utility>
#include <iostream>
#include <memory>

template <typename T>
struct DefaultDeleter {
	using type = std::default_delete<T>;
};

template <typename T>
struct DefaultDeleter<T[]> {
	using type = std::default_delete<T[]>;
};

// 处理非数组类型
template <typename T, typename Deleter = typename DefaultDeleter<T>::type>
class unique_ptr {
		T *ptr;
		Deleter deleter;

	public:

		explicit unique_ptr(T *p = nullptr) noexcept : ptr(p) {}
		~unique_ptr() {
			reset();
		}

		// 禁拷贝、赋值
		unique_ptr(const unique_ptr &) = delete;
		unique_ptr &operator=(const unique_ptr &) = delete;

		// 移动构造
		unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr), deleter(std::move(other.deleter)) {
			other.ptr = nullptr;
		}

		// 移动赋值
		unique_ptr &operator=(unique_ptr&& other) noexcept {
			if (this != &other) {
				reset();
				ptr = other.ptr;
				deleter = std::move(other.deleter);
				other.ptr = nullptr;
			}
			return *this;
		}

		T *release() noexcept {
			T* p = ptr;
			ptr = nullptr;
			return p;
		}

		void reset(T *p = nullptr) noexcept {
			if (ptr != p) {
				if (ptr)
					deleter(ptr);
				ptr = p;
			}
		}

		// 访问指针
		T *get() const noexcept {
			return ptr;
		}
		Deleter &get_deleter() noexcept {
			return deleter;
		}
		const Deleter &get_deleter() const noexcept {
			return deleter;
		}

		// 解引用
		T &operator*() const {
			return *ptr;
		}
		T *operator->() const noexcept {
			return ptr;
		}

		// 布尔转换
		explicit operator bool() const noexcept {
			return ptr != nullptr;
		}
};

// 偏特化，处理数组类型T[]
template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
		T *ptr;
		Deleter deleter;

	public:
		explicit unique_ptr(T *p = nullptr) noexcept : ptr(p) {}
		~unique_ptr() {
			reset();
		}

		// 禁拷贝、赋值
		unique_ptr(const unique_ptr &) = delete;
		unique_ptr &operator=(const unique_ptr &) = delete;

		// 移动构造
		unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr), deleter(std::move(other.deleter)) {
			other.ptr = nullptr;
		}

		// 移动赋值
		unique_ptr &operator=(unique_ptr&& other) noexcept {
			if (this != &other) {
				reset();
				ptr = other.ptr;
				deleter = std::move(other.deleter);
				other.ptr = nullptr;
			}
			return *this;
		}

		T *release() noexcept {
			T* p = ptr;
			ptr = nullptr;
			return p;
		}

		void reset(T *p = nullptr) noexcept {
			if (ptr != p) {
				if (ptr)
					deleter(ptr);
				ptr = p;
			}
		}

		// 访问指针
		T *get() const noexcept {
			return ptr;
		}
		Deleter &get_deleter() noexcept {
			return deleter;
		}
		const Deleter &get_deleter() const noexcept {
			return deleter;
		}

		// 布尔转换
		explicit operator bool() const noexcept {
			return ptr != nullptr;
		}

		// 数组下标访问
		T &operator[](size_t idx) const {
			return ptr[idx];
		}
};

int main() {
	unique_ptr<int> a(new int(2));
	std::cout << *a << std::endl;
	*a = 1;
	std::cout << *a << std::endl;
	unique_ptr<int[]> b(new int[3]);
	b[1] = 10;
	std::cout << b[1] << std::endl;
}
```
