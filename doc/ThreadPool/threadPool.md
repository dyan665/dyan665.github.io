# 线程池

[返回主页](../../README.md)

## 基本原理
> 创建一组工作线程作为线程池，线程共享一个任务队列，当任务队列不为空的时候，线程不停地取出任务执行，如果必要则提供同步手段，返回结果。
> 线程池为典型的生产者消费者模式，工作线程相当于消费者，不停消费任务、执行任务，线程池则为生产者，将任务不停添加到任务队列中等待工作线程的消费，因此也涉及到条件变量进行同步。


## 线程安全的任务队列
> 对于任务队列，由于涉及到多线程访问，因此采用`mutex`与`queue`实现线程安全的任务队列，提供的接口主要为`存任务`、`取任务`、`任务数量`。此处采用函数模板实现。
```cpp
template<class T> 
class SafeQueue { /* part 3.1: safe queue */
    public:
        SafeQueue() = default;
        SafeQueue(const SafeQueue& b){ // 拷贝构造
            unique_lock<mutex> lck(b._m);
            _q = b._q;
        }
        SafeQueue(SafeQueue && b){ // 移动构造
            unique_lock<mutex> lck(b._m);
            _q = move(b._q);
        }
        ~SafeQueue() = default;
        
        bool empty(){
            unique_lock<mutex> lck(_m);
            return _q.empty();
        }
        
        int size(){
            unique_lock<mutex> lck(_m);
            return _q.size();
        }
        
        void enqueue(T &&t){//非万能引用  必须在推导时才可能是万能引用 当声明该类时已经完成了推导 确认了T的类型 需要加下面左值引用的类型
            unique_lock<mutex> lck(_m);
            cout<<"rval enqueue"<<endl;
            _q.emplace(forward<T>(t));
        }
        
        void enqueue(T &t){//新加左值引用类型的函数
            unique_lock<mutex> lck(_m);
            cout<<"lval enqueue"<<endl;
            _q.emplace(t);
        }
        
        bool dequeue(T &t) {
            unique_lock<mutex> lck(_m);
            if(_q.empty())return false;
            t = move(_q.front());
            _q.pop();
            return true;
        }
        
    private:
        queue<T> _q;
        mutex _m;
    
};

```


## 工作线程
> 此处将工作线程包装为一个类，重载`operator()`，使其成为一个可运行对象，通过`thread`创建线程执行工作循环。
> 工作线程存在于线程池对象中，生命周期由线程池对象进行管理，因此，为了达到共享数据的目的，需要将线程池对象指针传入工作线程中，用以获取线程池的运行状态（正在运行还是停止）、同步等待任务到来、互斥获取任务队列中的新任务。
```cpp
class ThreadWorker{
        private:
            int id;
            ThreadPool *pool;
        public:
            ThreadWorker(ThreadPool *pool, const int id):pool(pool),id(id){}
            void operator()(){
                while(1){
                    bool rtn = false;
                    function<void()> t;
                    {
                        unique_lock<mutex> lck(pool->m); // 注意 此处能够访问ThreadPool的私有成员
//                      cout<<"before wait"<<endl;
                        pool->cv.wait(lck,[&](){
                            return !(pool->q).empty() || pool->_shutDown;
                        });
                        if((pool->q).empty() && pool->_shutDown)return;
                        rtn = pool->q.dequeue(t);   
                    }
                    if(rtn){
                        t();
                    }
                }
            }
    };
```

## 任务提交接口
> 任务提交接口的参数需要包括`普通函数`、`lambda函数`、`function对象`，因此需要基于模板实现。除此之外，为了包含不固定个数的参数，需要使用可变模板参数，使用完美转发传递左右值，



```cpp
template<class Fn, class... Args>
auto submit(Fn && f, Args&&... args) -> future<decltype(f(forward<Args>(args)...))> {
    //method 1
    function<decltype(f(args...))()> func = bind(forward<Fn>(f),forward<Args>(args)...);
    
    shared_ptr<packaged_task<decltype(f(args...))()>> task_ptr = make_shared<packaged_task<decltype(f(args...))()>>(func);
    //packaged_task<void()> task(func);
    function<void()> tmp = [task_ptr](){//用auto 推导出的为右值
        (*task_ptr)();
    }; 
    q.enqueue((tmp));
    cout<<"thread pool enqueue one task"<<flush<<endl;
    cv.notify_one();
    return task_ptr->get_future();
    
    //method 2          
    
    return future<decltype(f(args...))>();
}
```


## 完整代码
```cpp
/* part 3: thread pool */

template<class T> 
class SafeQueue { /* part 3.1: safe queue */
    public:
        SafeQueue() = default;
        SafeQueue(const SafeQueue& b){ // 拷贝构造
            unique_lock<mutex> lck(b._m);
            _q = b._q;
        }
        SafeQueue(SafeQueue && b){ // 移动构造
            unique_lock<mutex> lck(b._m);
            _q = move(b._q);
        }
        ~SafeQueue() = default;
        
        bool empty(){
            unique_lock<mutex> lck(_m);
            return _q.empty();
        }
        
        int size(){
            unique_lock<mutex> lck(_m);
            return _q.size();
        }
        
        void enqueue(T &&t){//非万能引用  必须在推导时才可能是万能引用 当声明该类时已经完成了推导 确认了T的类型 需要加下面左值引用的类型
            unique_lock<mutex> lck(_m);
            cout<<"rval enqueue"<<endl;
            _q.emplace(forward<T>(t));
        }
        
        void enqueue(T &t){//新加左值引用类型的函数
            unique_lock<mutex> lck(_m);
            cout<<"lval enqueue"<<endl;
            _q.emplace(t);
        }
        
        bool dequeue(T &t) {
            unique_lock<mutex> lck(_m);
            if(_q.empty())return false;
            t = move(_q.front());
            _q.pop();
            return true;
        }
        
    private:
        queue<T> _q;
        mutex _m;
    
};

class ThreadPool {  
    private:
    class ThreadWorker{
        private:
            int id;
            ThreadPool *pool;
        public:
            ThreadWorker(ThreadPool *pool, const int id):pool(pool),id(id){}
            void operator()(){
                while(1){
                    bool rtn = false;
                    function<void()> t;
                    {
                        unique_lock<mutex> lck(pool->m); // 注意 此处能够访问ThreadPool的私有成员
//                      cout<<"before wait"<<endl;
                        pool->cv.wait(lck,[&](){
                            return !(pool->q).empty() || pool->_shutDown;
                        });
                        if((pool->q).empty() && pool->_shutDown)return;
                        rtn = pool->q.dequeue(t);   
                    }
                    if(rtn){
                        t();
                    }
                }
            }
    };
    

        bool _shutDown;
        SafeQueue<function<void()>> q;
        vector<thread> threads;
        mutex m;
        condition_variable cv;
    
    public :
        
        ThreadPool(int num = 5):threads(vector<thread>(num)), _shutDown(false){}
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool(ThreadPool &&) = delete;
        ThreadPool& operator=(const ThreadPool &) = delete;
        ThreadPool& operator=(ThreadPool &&) = delete;  
        ~ThreadPool(){shutDown();}
        void init(){
            int cnt = 0;
            for(auto& t:threads){
                t = thread(ThreadWorker(this,cnt++));
            }
        }
        void shutDown(){
            _shutDown = true;
            cv.notify_all();
            for(auto& t:threads) {
                if(t.joinable())t.join();
            }
        }
        
        template<class Fn, class... Args>
        auto submit(Fn && f, Args&&... args) -> future<decltype(f(forward<Args>(args)...))> {
            //method 1
            function<decltype(f(args...))()> func = bind(forward<Fn>(f),forward<Args>(args)...);
            
            shared_ptr<packaged_task<decltype(f(args...))()>> task_ptr = make_shared<packaged_task<decltype(f(args...))()>>(func);
            //packaged_task<void()> task(func);
            function<void()> tmp = [task_ptr](){//用auto 推导出的为右值
                (*task_ptr)();
            }; 
            q.enqueue((tmp));
            cout<<"thread pool enqueue one task"<<flush<<endl;
            cv.notify_one();
            return task_ptr->get_future();
            
            //method 2          
            
            return future<decltype(f(args...))>();
        }
        
        
};

int main(){
    ThreadPool t(10);
    t.init();
    auto func = [](int x,int time){
        while(time--){
            cout<<x<<"-"<<time<<flush<<endl;
        }
    };
    auto t1 = t.submit(func, 0,2);
    auto t2 = t.submit(func, 1,2);
    auto t3 = t.submit(func, 2222,5);
    thread(func,3,4).detach();
//  this_thread::sleep_for(chrono::seconds(2));
    t1.get();
    t2.get();
    t3.get();
    t.shutDown();
    return 0;
}
```



