# 插件技术


# 原理
1. 基于dlopen、dlclose支持动态加载、卸载动态库，基于dlsym查找动态库中导出的对外接口；
2. 对全部插件定义统一接口PluginBase，将其接口作为外部调用API，除此之外，对于每一个插件，必须存在统一的导出函数接口CreatePluginFunc与DestroyPluginFunc来创建、销毁插件，其余符号则对外部不可见，考虑到ABI兼容性，导出函数接口统一为c函数类型；
3. 为了统一管理全部插件资源防止资源泄漏，设置PluginManager类，为了支持插件的热更新，使用独立线程定时监测插件创建时间是否更新，更新则关闭旧插件，打开新插件，并更新对应的创建、销毁接口；

# 实现
## plugin_interface插件接口

```cpp
#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

#define PLUGIN_API __attribute__((visibility("default")))

class PluginBase {
public:
    virtual ~PluginBase() = default;
    
    virtual bool open() = 0;
    virtual bool setConfig(const std::unordered_map<std::string, std::string>& config) = 0;
    virtual std::string inference(const std::string& input) = 0;
    virtual bool close() = 0;
    
    // 异步通知热重载接口  插件管理器通知插件实例已准备热重载 需重新获取新插件实例
    virtual bool prepareForReload() = 0;
};

// 插件导出接口
using CreatePluginFunc = PluginBase* (*)();
using DestroyPluginFunc = void (*)(PluginBase*);

#endif // PLUGIN_INTERFACE_H

```

## plugin_manager插件管理器

```cpp
#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_interface.h"
#include <dlfcn.h>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>

class PluginManager {
public:
    PluginManager() : stop_monitor(false) {}
    
    ~PluginManager() {
        stopMonitoring();
        unloadAll();
    }

    // 加载插件
    bool load(const std::string& pluginPath, const std::string& pluginName) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (handles.find(pluginName) != handles.end()) {
            return false; // 已加载
        }

        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            throw std::runtime_error(dlerror());
        }

        auto create = reinterpret_cast<CreatePluginFunc>(dlsym(handle, "createPlugin"));
        auto destroy = reinterpret_cast<DestroyPluginFunc>(dlsym(handle, "destroyPlugin"));

        if (!create || !destroy) {
            dlclose(handle);
            return false;
        }

        PluginHandle ph;
        ph.handle = handle;
        ph.create = create;
        ph.destroy = destroy;
        ph.path = pluginPath;
        ph.last_mtime = getFileMtime(pluginPath);

        handles[pluginName] = ph;
        
        // 如果监控线程未启动，启动它
        if (!monitor_thread.joinable()) {
            startMonitoring();
        }
        
        return true;
    }

    // 获取插件实例
    std::shared_ptr<PluginBase> getInstance(const std::string& pluginName) {
        std::lock_guard<std::mutex> lock(mtx);
        if (auto it = handles.find(pluginName); it != handles.end()) {
            auto deleter = [this, pluginName](PluginBase* p) {
                std::lock_guard<std::mutex> lock(mtx);
                if (auto it = handles.find(pluginName); it != handles.end()) {
                    p->close();
                    it->second.destroy(p);
                    // 从活跃实例列表中移除
                    auto& instances = it->second.active_instances;
                    instances.erase(
                        std::remove_if(instances.begin(), instances.end(),
                            [](const auto& weak_ptr) { return weak_ptr.expired(); }),
                        instances.end());
                }
            };
            PluginBase* raw_ptr = it->second.create();
            std::shared_ptr<PluginBase> instance(raw_ptr, deleter);
            // 记录活跃实例
            it->second.active_instances.emplace_back(instance);
            return instance;
        }
        return nullptr;
    }

    // 外部主动触发插件重载检查
    void checkForReload() {
        std::lock_guard<std::mutex> lock(mtx);
        checkPluginsForReload();
    }

private:
    struct PluginHandle {
        void* handle;
        CreatePluginFunc create;
        DestroyPluginFunc destroy;
        std::string path;
        time_t last_mtime;
        
        // 跟踪当前活跃插件实例
        std::vector<std::weak_ptr<PluginBase>> active_instances;
    };

    std::unordered_map<std::string, PluginHandle> handles;
    std::mutex mtx;
    std::thread monitor_thread;
    std::atomic<bool> stop_monitor;
    std::condition_variable cv;

    time_t getFileMtime(const std::string& path) {
        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0) {
            return statbuf.st_mtime;
        }
        return 0;
    }

    void startMonitoring() {
        stop_monitor = false;
        monitor_thread = std::thread([this]() {
            while (!stop_monitor) {
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    checkPluginsForReload();
                }
                
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock, std::chrono::seconds(2), [this]() {
                    return stop_monitor.load();
                });
            }
        });
    }

    void stopMonitoring() {
        stop_monitor = true;
        cv.notify_all();
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
    }

    void checkPluginsForReload() {
        for (auto it = handles.begin(); it != handles.end(); ) {
            time_t current_mtime = getFileMtime(it->second.path);
            if (current_mtime > it->second.last_mtime) {
                std::cout << "Detected change in plugin: " << it->first 
                          << ", preparing to reload..." << std::endl;
                
                // 通知所有活跃实例准备重载新插件 让活跃实例重新通过插件管理器获取新插件对象
                bool can_reload = true;
                for (auto& weak_instance : it->second.active_instances) {
                    if (auto instance = weak_instance.lock()) {
                        if (!instance->prepareForReload()) {
                            can_reload = false;
                            break;
                        }
                    }
                }
                
                if (!can_reload) {
                    std::cerr << "Some instances are not ready for reload, skipping" << std::endl;
                    ++it;
                    continue;
                }
                
                // 加载新版本插件
                try {
                    void* new_handle = dlopen(it->second.path.c_str(), RTLD_LAZY | RTLD_LOCAL);
                    if (!new_handle) throw std::runtime_error(dlerror());
                    
                    auto new_create = reinterpret_cast<CreatePluginFunc>(dlsym(new_handle, "createPlugin"));
                    auto new_destroy = reinterpret_cast<DestroyPluginFunc>(dlsym(new_handle, "destroyPlugin"));
                    
                    if (!new_create || !new_destroy) {
                        dlclose(new_handle);
                        throw std::runtime_error("Invalid plugin symbols");
                    }
                    
                    // 替换插件接口
                    PluginHandle new_handle_info = it->second;
                    new_handle_info.handle = new_handle;
                    new_handle_info.create = new_create;
                    new_handle_info.destroy = new_destroy;
                    new_handle_info.last_mtime = current_mtime;
                    
                    // 关闭旧插件 但不销毁活跃实例
                    dlclose(it->second.handle);
                    
                    // 5. 更新插件信息
                    it->second = new_handle_info;
                    
                    std::cout << "Successfully reloaded plugin: " << it->first << std::endl;
                    ++it;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to reload plugin " << it->first 
                              << ": " << e.what() << std::endl;
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    void unloadAll() {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& [name, handle] : handles) {
            dlclose(handle.handle);
        }
        handles.clear();
    }
};

#endif // PLUGIN_MANAGER_H
```

## sample_plugin示例插件

```cpp
#ifndef SAMPLE_PLUGIN_H
#define SAMPLE_PLUGIN_H

#include "plugin_interface.h"
#include <iostream>
#include <atomic>

class PLUGIN_API SamplePlugin : public PluginBase {
    std::atomic<bool> reload_prepared{false};
    
public:
    SamplePlugin() {
        std::cout << "SamplePlugin constructed\n";
    }
    
    ~SamplePlugin() {
        std::cout << "SamplePlugin destroyed\n";
    }

    bool open() override {
        if (reload_prepared) {
            std::cout << "SamplePlugin reopened after reload\n";
            reload_prepared = false;
        } else {
            std::cout << "SamplePlugin opened\n";
        }
        return true;
    }

    bool setConfig(const std::unordered_map<std::string, std::string>& config) override {
        for (const auto& [key, value] : config) {
            std::cout << "Set config: " << key << " = " << value << "\n";
        }
        return true;
    }

    std::string inference(const std::string& input) override {
        if (allow_reload) {
            throw std::runtime_error("Plugin is being reloaded");
        }
        return "inference: " + input + " done.";
    }

    bool close() override {
        std::cout << "SamplePlugin closed\n";
        return true;
    }
    
    bool prepareForReload() override {
        if (/* 检查是否可以安全重载 */) {
            allow_reload = true;
            return true;
        }
        return false;
    }
};

// 导出符号（带可见性控制）
extern "C" {
    PLUGIN_API PluginBase* createPlugin() {
        return new SamplePlugin();
    }
    
    PLUGIN_API void destroyPlugin(PluginBase* p) {
        delete p;
    }
}

#endif // SAMPLE_PLUGIN_H

//g++ -fPIC -shared -fvisibility=hidden sample_plugin.cpp -o libsample_plugin.so -ldl

```

[返回主页](../../README.md)
