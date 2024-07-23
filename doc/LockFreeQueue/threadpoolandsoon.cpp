#include<iostream>
#include<vector>
#include<algorithm>
#include<string>
#include<functional>
#include<mutex>
#include<condition_variable>
#include<atomic>
#include<thread>
#include<limits>
#include<memory>
#include<unordered_map>
#include<map>
#include<sstream>
#include<chrono>
#include<queue>
#include<future>

using namespace std;


//class Single{
//	public:
//		static Single & get(){
//			return s;
//		}
//	
//	private:
//		Single();
//		~Single();
//		static Single s;
//		Single(const Single&);
//		Single(Single&&) = delete;
//		Single& operator=(const Single&);
//};
//Single Single::s;
//
//
//// version 1.3
//class Singleton
//{
//private:
//	static Singleton instance;
//private:
//	Singleton();
//	~Singleton();
//	Singleton(const Singleton&);
//	Singleton& operator=(const Singleton&);
//public:
//	static Singleton& getInstance() {
//		return instance;
//	}
//};
//
//// initialize defaultly
//Singleton Singleton::instance;





//int main1(void){
////	auto s0 = Singleton::getInstance();
//	auto& s1 = Single::get();
//	auto& s2 = Single::get();
//	
//	cout<<"single address:"<<&s1 <<" " << &s2 << endl;
//	
//	
//	
//	return 0;	
//}


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




/* part 2: producer + consumer */

atomic<bool> stop(false);
const int V_MAX = 20;
vector<string> v;
mutex myMutex;
condition_variable vProduceOne, vNeedOne;


void producer(){
	static int cnt = 0 ;
	while(!stop){
		unique_lock<mutex> lck(myMutex);
		vNeedOne.wait(lck,[]{
			return v.size() < V_MAX || stop;
		});
		if(stop)break;
		stringstream tmp;
		tmp << this_thread::get_id() << "_" << to_string(cnt++);
		string s;
		tmp >> s;
		v.emplace_back(s);
		cout << this_thread::get_id() << " produce " << cnt-1 << endl;
		vProduceOne.notify_one();
	}
}


void consumer(){
	while(true) {
		unique_lock<mutex> lck(myMutex);
		vProduceOne.wait(lck,[]{
			return !v.empty() || stop;
		});
		if(v.empty()){
			break;	
		}
		auto t = v.back();
		v.pop_back();
		cout << "consume "<< t<<endl;
		vNeedOne.notify_one();
	}
}


int main2(void){
	//consumer
	vector<thread> consumers;
	for(int i=0;i<10;++i){
		consumers.emplace_back(consumer);
	}
	cout<<"wait 3s after create consumer theads." << endl;
	this_thread::sleep_for(chrono::seconds(3));
	
	//producer
	vector<thread> producers;
	for(int i=0;i<2;++i){
		producers.emplace_back(producer);
	}
	cout<<"wait 3s after create produer theads." << endl;
	this_thread::sleep_for(chrono::seconds(1));
	
	//stop
	stop = true;
	vProduceOne.notify_all();
	vNeedOne.notify_all();
	
	for(auto&t:consumers)t.join();
	for(auto&t:producers)t.join();
	
	return 0;
}



/* part 3: thread pool */

template<class T> 
class SafeQueue { /* part 3.1: safe queue */
	public:
		SafeQueue() = default;
		SafeQueue(const SafeQueue& b){
			unique_lock<mutex> lck(b._m);
			_q = b._q;
		}
		SafeQueue(SafeQueue && b){
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
						unique_lock<mutex> lck(pool->m);
//						cout<<"before wait"<<endl;
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
			packaged_task<decltype(f(args...))()> task(func);
			//packaged_task<void()> task(func);
			function<void()> tmp = [&task](){//用auto 推导出的为右值
				(task)();
			}; 
			q.enqueue((tmp));
			cout<<"thread pool enqueue one task"<<flush<<endl;
			cv.notify_one();
			return task.get_future();
			
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
//	this_thread::sleep_for(chrono::seconds(2));
	t1.get();
	t2.get();
	t3.get();
	t.shutDown();
	return 0;
}
























/* part 4: unique ptr */

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
//	cout << t << endl;
//}


int main4(void){
	UniquePtr<int> test1(new int(2));
	UniquePtr<int> test11(move(test1));
	
	UniquePtr<int[]> test2(new int[2]{1,2});
	int test3[] = {3,4};
	int * a = test3;
	UniquePtr<int[]> test4(test3);
	UniquePtr<int[]> test5(a);
	
//	func(1212);
//	func("string12");
	
	return 0;
}




/* part 5: share ptr */










/* part 6: endian */


struct endian{
	uint32_t a:8;
	uint32_t b:8;
	uint32_t c:8;
	uint32_t d1:1;
	uint32_t d2:2;
	uint32_t d3:3;
	uint32_t d4:2;
};
struct endian2{
	uint32_t d1:7;
	uint32_t d2:3;
	uint32_t d3:3;
	uint32_t d4:2;
};
struct endian3{
	uint8_t d1:1;
	uint8_t d2:2;
	uint8_t d3:3;
	uint8_t d4:2;
};


int main6(){
	#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	cout <<"little endian" <<endl;
	#elif __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	cout <<"big endian"<<endl;
	#endif
	//little endian
	uint32_t t = 0X12345678;
	//0001 0010 0011 0100 0101 0110 0111 1000     读出顺序
	uint8_t *pt = (uint8_t*)&t;
	cout << hex << (unsigned)pt[0] << " " << (unsigned)pt[1] << " " << (unsigned)pt[2] << " " << (unsigned)pt[3] << endl;//78 56 34 12
	endian *en = (endian*)&t;
	cout <<hex  << en->a << " "  << en->b << " "  << en->c <<endl;//78 56 34
	cout << en->d1 <<" "<<en->d2<<" " <<en->d3 <<" " <<en->d4<<endl;//0 1 2 0
	
	endian2 *en2 = (endian2*)&t;
	cout << en2->d1 <<" "<<en2->d2<<" " <<en2->d3 <<" " <<en2->d4<<endl;//78 4 5 2
	
	endian3 *en3 = (endian3*)pt;//78
	cout <<(unsigned) en3->d1 <<" "<<(unsigned)en3->d2<<" " <<(unsigned)en3->d3 <<" " <<(unsigned)en3->d4<<endl;//0 0 7 1
	/*
		结论：大小端针对的字节序，对于字节内还是按照从高位到低位的排列顺序
		对于结构体位域，则按大小端排列即可。
		比如，首先看大小端（比如上述为小端），然后将uint32_t在排列好(按照读出的顺序)，然后看结构体，按小端倒着排，有几位占几位，然后按正序看值。比如endian3中d4，占两位，对于内存中就是占最高两位，也就是01，所以d4的值就是1
	*/
	return 0;
}







/* part 7: 访问者模式 visitor mode 行为模式 */
/* 目的：数据与操作分离。被访问者为不同的数据，存在较多的访问者，被访问者一般只存储数据，具体操作则在访问者内，主要针对的是，相同访问者针对不同被访问者时存在不同的操作 */
class AbstractData;
class AbstractVistor{
	public:
		virtual void visit(AbstractData*){
			return;
		}
};

class AbstractData {
	public:
		int count;
		string name;
		virtual void accept(AbstractVistor *vis){
			vis->visit(this);
		}
};

class DataA : public AbstractData {
	
};




class VisitorMode{
	public:
	
};









/* part 8: 观察者模式 */

class AbstractObserver {
	public:
		AbstractObserver(){}
		AbstractObserver(int id):_id(id){}
		virtual void onChange(int a){
			cout << "No." << _id << " AbstractObserver get a change " << a <<endl;
		}
		int testFunc(){
			return test;
		}
	protected:
		int _id = 0;
	private:
		int test = 11;
};

class DeriveObserver: public AbstractObserver{
	public:
		DeriveObserver(int a):AbstractObserver(a){}
		void onChange(int a) override {
			cout << "No." << _id << " DeriveObserver get a change " << a <<endl;
		}
		int testFunc(){
			return test;
		}
	private:
		int test = 22;
};

class AbstractMaster {
	public:
		virtual void addObserver(const shared_ptr<AbstractObserver> &b) {
			_v.push_back(b);
		}
		virtual bool rmvObserver(const shared_ptr<AbstractObserver> &b) {
			for(auto begin = _v.begin();begin!=_v.end();begin++){
				if(*begin == b) { // shared_ptr operator==  检查的是内部的指针地址是否相等
					_v.erase(begin);
					return true;
				}
			}
			return false;
		}
		virtual void change(int a) {
			for(auto begin = _v.begin();begin!=_v.end();begin++){
				(*begin)->onChange(a);
			}
		}
		
	private:
	vector<shared_ptr<AbstractObserver>> _v;
	
};


int main8(){
	AbstractMaster master;
	auto ptr1 = make_shared<AbstractObserver>();
	master.addObserver(ptr1);
	auto ptr2 = make_shared<AbstractObserver>(1);
	master.addObserver(ptr2);
	master.change(20);
	master.rmvObserver(ptr1);
	shared_ptr<DeriveObserver> ptr3 = make_shared<DeriveObserver>(2);
//	auto ptr3 = make_shared<DeriveObserver>(2);
	master.addObserver(ptr3);// 会构造一个shared_ptr<AbstractObserver>的右值  因此参数类型必须为const 防止修改右值 
	master.change(200);
	
	DeriveObserver tmpDe(3);
	AbstractObserver & tmp = tmpDe;
	tmp.onChange(1111);
	cout<< tmp.testFunc() <<" "<<((DeriveObserver&)tmp).testFunc() << endl;
	return 0;
}


/* part 9: 堆排 */







/* part 10: taskflow */








/* part 11: 读写锁 */






/* part 12: 自旋锁 */






/* part 13: 无锁消息队列 1生产者1消费者  */

//struct kfifo { 
//    unsigned char *buffer;    /* the buffer holding the data */ 
//    unsigned int size;    /* the size of the allocated buffer */ 
//    unsigned int in;    /* data is added at offset (in % size) */ 
//    unsigned int out;    /* data is extracted from off. (out % size) */ 
//    spinlock_t *lock;    /* protects concurrent modifications */ 
//};
//
//struct kfifo *kfifo_alloc(unsigned int size, gfp_t gfp_mask, spinlock_t *lock) 
//{ 
//    unsigned char *buffer; 
//    struct kfifo *ret; 
//
//    /* 
//     * round up to the next power of 2, since our 'let the indices 
//     * wrap' tachnique works only in this case. 
//     */ 
//    if (size & (size - 1)) { 
//        BUG_ON(size > 0x80000000); 
//        size = roundup_pow_of_two(size); 
//    } 
//
//    buffer = kmalloc(size, gfp_mask); 
//    if (!buffer) 
//        return ERR_PTR(-ENOMEM); 
//
//    ret = kfifo_init(buffer, size, gfp_mask, lock); 
//
//    if (IS_ERR(ret)) 
//        kfree(buffer); 
//
//    return ret; 
//} 
//
//unsigned int __kfifo_put(struct kfifo *fifo, 
//             unsigned char *buffer, unsigned int len) 
//{ 
//    unsigned int l; 
//
//    len = min(len, fifo->size - fifo->in + fifo->out); 
//
//    /* 
//     * Ensure that we sample the fifo->out index -before- we 
//     * start putting bytes into the kfifo. 
//     */ 
//
//    smp_mb(); 
//
//    /* first put the data starting from fifo->in to buffer end */ 
//    l = min(len, fifo->size - (fifo->in & (fifo->size - 1))); 
//    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l); 
//
//    /* then put the rest (if any) at the beginning of the buffer */ 
//    memcpy(fifo->buffer, buffer + l, len - l); 
//
//    /* 
//     * Ensure that we add the bytes to the kfifo -before- 
//     * we update the fifo->in index. 
//     */ 
//
//    smp_wmb(); 
//
//    fifo->in += len; 
//
//    return len; 
//}
//
//unsigned int __kfifo_get(struct kfifo *fifo, 
//             unsigned char *buffer, unsigned int len) 
//{ 
//    unsigned int l; 
//
//    len = min(len, fifo->in - fifo->out); 
//
//    /* 
//     * Ensure that we sample the fifo->in index -before- we 
//     * start removing bytes from the kfifo. 
//     */ 
//
//    smp_rmb(); 
//
//    /* first get the data from fifo->out until the end of the buffer */ 
//    l = min(len, fifo->size - (fifo->out & (fifo->size - 1))); 
//    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l); 
//
//    /* then get the rest (if any) from the beginning of the buffer */ 
//    memcpy(buffer + l, fifo->buffer, len - l); 
//
//    /* 
//     * Ensure that we remove the bytes from the kfifo -before- 
//     * we update the fifo->out index. 
//     */ 
//
//    smp_mb(); // read(load) write(store) barrier
//
//    fifo->out += len; 
//
//    return len; 
//} 



/* part 14: 内存屏障 */

#define lfence() __asm__ __volatile__("lfence": : :"memory") 
#define sfence() __asm__ __volatile__("sfence": : :"memory") 
#define mfence() __asm__ __volatile__("mfence": : :"memory") 


int main14(void) {
	lfence();
	sfence();
	mfence();
	return 0;
}



/* part 15: SPSC https://luyuhuang.tech/2022/10/30/lock-free-queue.html */

template<class T, size_t capSize>
class spsc : private allocator<T> {
	public:
		spsc():_addr(allocator<T>::allocate(capSize)){}
		spsc(const spsc&) =delete;
		spsc(spsc &&) =delete;
		spsc &operator= (const spsc&) = delete;
		spsc &operator= (spsc&&) = delete;
		~spsc(){
			allocator<T>::deallocate(_addr, capSize);
		}
		
		template<class ...Args>
		bool emplace(Args && ...args) {
			size_t h = head.load();
			if((h+1)%capSize == tail.load()){
//				cout<<head.load()<<" "<<tail.load()<<endl;
				return false;// full
			}
			allocator<T>::construct(_addr + h, forward<Args>(args)...);
			head.store((h+1)%capSize);
//			cout<<"head add " << head.load();
			return true;
		}
		
		bool pop(T& tt){
			size_t t = tail.load();
			if(t == head.load()){
				return false;//empty
			}
			tt = move(_addr[t]);
			allocator<T>::destroy(_addr+t);
			tail.store((t+1)%capSize);
			return true;
		}	
		
	private:
		T * _addr = nullptr;
		atomic<size_t> head{0};
		atomic<size_t> tail{0};
};

template<class T, size_t capSize>
class spsc2 : private allocator<T> {
	public:
		spsc2():_addr(allocator<T>::allocate(capSize)){}
		spsc2(const spsc2&) =delete;
		spsc2(spsc2 &&) =delete;
		spsc2 &operator= (const spsc2&) = delete;
		spsc2 &operator= (spsc2&&) = delete;
		~spsc2(){
			allocator<T>::deallocate(_addr, capSize);
		}
		
		template<class ...Args>
		bool emplace(Args && ...args) {
			size_t h = head.load(memory_order_relaxed);
			if((h+1)%capSize == tail.load(memory_order_acquire)){
//				cout<<head.load()<<" "<<tail.load()<<endl;
				return false;// full
			}
			allocator<T>::construct(_addr + h, forward<Args>(args)...);
			head.store((h+1)%capSize,memory_order_release);
//			cout<<"head add " << head.load();
			return true;
		}
		
		bool pop(T& tt){
			size_t t = tail.load(memory_order_relaxed);
			if(t == head.load(memory_order_acquire)){
				return false;//empty
			}
			tt = move(_addr[t]);
			allocator<T>::destroy(_addr+t);
			tail.store((t+1)%capSize,memory_order_release);
			return true;
		}	
		
	private:
		T * _addr = nullptr;
		atomic<size_t> head{0};
		atomic<size_t> tail{0};
};

//#define _GNU_SOURCE
//#include <pthread.h>
//
//void pinThread(int cpu) {
//  if (cpu < 0) {
//    return;
//  }
//  cpu_set_t cpuset;
//  CPU_ZERO(&cpuset);
//  CPU_SET(cpu, &cpuset);
//  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
//      -1) {
//    perror("pthread_setaffinity_no");
//    exit(1);
//  }
//}

template <typename T, size_t Cap>
class spsc1 : private allocator<T> {
    T *data;
    atomic<size_t> head{0}, tail{0};
public:
    spsc1(): data(allocator<T>::allocate(Cap)) {}
    spsc1(const spsc1&) = delete;
    spsc1 &operator=(const spsc1&) = delete;
    spsc1 &operator=(const spsc1&) volatile = delete;

    bool push(const T &val) {
        return emplace(val);
    }
    bool push(T &&val) {
        return emplace(std::move(val));
    }

    template <typename ...Args>
    bool emplace(Args && ...args) { // 入队操作
        size_t t = tail.load(memory_order_relaxed);
        if ((t + 1) % Cap == head.load(memory_order_acquire)) // (1)
            return false;
        allocator<T>::construct(data + t, std::forward<Args>(args)...);
        // (2)  synchronizes-with (3)
        tail.store((t + 1) % Cap, memory_order_release); // (2)
        return true;
    }

    bool pop(T &val) { // 出队操作
        size_t h = head.load(memory_order_relaxed);
        if (h == tail.load(memory_order_acquire)) // (3)
            return false;
        val = std::move(data[h]);
        allocator<T>::destroy(data + h);
        // (4) synchronizes-with (1)
        head.store((h + 1) % Cap, memory_order_release); // (4)
        return true;
    }
};



int main15(void){
	const size_t queueSize = 10000000;
  	const int64_t iters = 10000000;
  	
	spsc<int,queueSize> spc;
	thread in,out;
	int max_num = iters;
	auto start1 = chrono::steady_clock::now();
	in = thread([&](){
//		pinThread(0);
		for(int i=0;i<max_num;++i){
			while(!spc.emplace(i)){
//				cout<<"push fail"<<endl;
			};
		}
	});
	out = thread([&](){
//		pinThread(1);
		int tmp;
		for(int i=0;i<max_num;++i){
			while(!spc.pop(tmp));
//			cout<<tmp<<endl;
		}
	});
	in.join();
	out.join();
	auto end1 = chrono::steady_clock::now();
	cout << (long long )max_num * 1000000 / chrono::duration_cast<chrono::nanoseconds>(end1-start1).count() << "ops/ms"<<endl;//14737ops/ms
	
	spsc2<int,queueSize> spc2;
	start1 = chrono::steady_clock::now();
	thread in2 = thread([&](){
//		pinThread(0);
		for(int i=0;i<max_num;++i){
			while(!spc.emplace(i)){
//				cout<<"push fail"<<endl;
			};
		}
	});
	thread out2 = thread([&](){
//		pinThread(1);
		int tmp;
		for(int i=0;i<max_num;++i){
			while(!spc.pop(tmp));
//			cout<<tmp<<endl;
		}
	});
	in2.join();
	out2.join();
	end1 = chrono::steady_clock::now();
	cout << (long long )max_num * 1000000 / chrono::duration_cast<chrono::nanoseconds>(end1-start1).count() << "ops/ms"<<endl;//14737ops/ms
	
	return 0;
}

/*
	rigtorp SPSCQueue				: 15417 ops/ms
	spsc(acquire release同步)		:	14737ops/ms
	spsc 顺序一致性				:		

*/



/* part16：forward move remove_reference 的实现 */
// https://zhuanlan.zhihu.com/p/580797507

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
	
	// forward
	template<class T>
	T&& forward(typename yy::remove_reference<T>::type& t) {// perfect forward for lvalue
		cout << "T&" <<endl;
		return static_cast<T&&>(t);
	}
	template<class T>
	T&& forward(typename yy::remove_reference<T>::type&& t) {// perfect forward for rvalue
		cout << "T&&" <<endl;
	 	return static_cast<T&&>(t);
	}
}


int main16(void) {
	
	int a = 1;
	int&& b = 1;
	// forward一般用于模板中，普通的貌似不起作用
//	std::forward<decltype(a)>(a);
	
	yy::forward<decltype(a)>(a);
	yy::forward<decltype(b)>(b);
	yy::forward<decltype(yy::move(a))>(yy::move(a));
	return 0;
}


#include <iostream>
#include <type_traits>
using namespace std;

template <typename _Tp>
constexpr _Tp &&
forward_(typename std::remove_reference<_Tp>::type &__t) noexcept
{
    cout << "=== forward_(typename std::remove_reference<_Tp>::type &__t)" << endl;
    if (std::is_same<_Tp, int>::value)
        cout << "_Tp type: int" << endl;
    if (std::is_same<_Tp, int &>::value)
        cout << "_Tp type: int &" << endl;
    if (std::is_same<_Tp, int &&>::value)
        cout << "_Tp type: int &&" << endl;
    return static_cast<_Tp &&>(__t);
}

template <typename _Tp>
constexpr _Tp &&
forward_(typename std::remove_reference<_Tp>::type &&__t) noexcept
{
    cout << "=== forward_(typename std::remove_reference<_Tp>::type &&__t)" << endl;
    if (std::is_same<_Tp, int>::value)
        cout << "_Tp type: int" << endl;
    if (std::is_same<_Tp, int &>::value)
        cout << "_Tp type: int &" << endl;
    if (std::is_same<_Tp, int &&>::value)
        cout << "_Tp type: int &&" << endl;
    static_assert(!std::is_lvalue_reference<_Tp>::value, "template argument"
                                                         " substituting _Tp is an lvalue reference type");
    return static_cast<_Tp &&>(__t);
}

// ===============================================================

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
    foo(forward_<T>(x));
}

template <typename _Tp>
void fff(typename remove_reference<_Tp>::type& x)
{
    cout << "=== fff(T& x)" << endl;
    if (std::is_same<_Tp, int>::value)
        cout << "_Tp type: int" << endl;
    if (std::is_same<_Tp, int &>::value)
        cout << "_Tp type: int &" << endl;
    if (std::is_same<_Tp, int &&>::value)
        cout << "_Tp type: int &&" << endl;
}

//template <typename _Tp>
//void fff(_Tp&& x)
//{
//    cout << "=== fff(T&& x)" << endl;
//    if (std::is_same<_Tp, int>::value)
//        cout << "_Tp type: int" << endl;
//    if (std::is_same<_Tp, int &>::value)
//        cout << "_Tp type: int &" << endl;
//    if (std::is_same<_Tp, int &&>::value)
//        cout << "_Tp type: int &&" << endl;
//}

void test(int &a){
	if(a == 2 )cout<<"a == 2"<<endl;
	a = 3;
	return;
}

int main16_1()
{
    int a = 1;
    int &b = a;
    int &&c = 2;

    bar(123); // 怎么调到forward第一个模板那去了？
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int
    // rvalue: 123

    bar(a);
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 1

    bar(b);
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 1

    bar(c);
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int &
    // lvalue: 2

    bar(std::move(a));
    // === forward_(typename std::remove_reference<_Tp>::type &__t)
    // _Tp type: int
    // rvalue: 1
    
//    fff(123);
    fff<decltype(c)>(c);
//    fff(b);
//    fff(c);
//    fff(move(a));
//	test(c);cout <<c<<endl;

//	forward(c);//error

    return 0;
}


/* part18：有限状态机 */

//typedef enum {
//	STATUS_1 = 0,
//	STATUS_2,
//	STATUS_3,
//	STATUS_4,
//	STATUS_NONE
//}STATUS;
//
//typedef enum {
//	EVENT_1 = 0,
//	EVENT_2,
//	EVENT_3,
//	EVENT_4,
//	EVENT_NONE
//}EVENT;
//
//typedef void(*CALLBACK)(void* session/*save all status*/,void* input);
//
//typedef struct{
//	bool isValid;
//	STATUS status;
//	EVENT event;
//	CALLBACK callbackfunc;
//}fsmStruct;
//
//fsmStruct fsm[STATUS_NONE][EVENT_NONE] = {};
//
//bool register(STATUS s, EVENT e, CALLBACK callback){
//	//check s and e and callback
//	fsm[s][e] = {1,s,e,callback};
//	return true;
//}
//
//void callbackexample(void* session, void* input){//STATUS_1 when get EVENT 1
//	//read session
//	//process input and update session
//	//change STATUS and save it in session
//	return;
//}
//
//void dispatch_event(session, msg, eventtype) {
//	auto callback = fsm(session->status, eventtype);
//	callback(session, msg);
//}
//
//int main(void) {
//	//for loop
//	while(true){
//		//socket wait for event
//		//epoll()
//		//when rcv a msg, check msg type, according to now status, get event type
//		getsession(msg);//get seesion
//		eventtype = geteventtype(session, msg);
//		dispatch_event(session, msg, eventtype);
//	}
//	return 0;
//}



/* part18：mailbox actor并发模型 */
// done




/* part19：读写锁 */
struct flexStruct{
	int aa;
	int b;
	int size;
	char c[0];
	flexStruct(int size){
		this->size = size - sizeof(flexStruct);
	}
	virtual bool test(){
		return true;
	}
};
//
int main19(void){
	char buf[100];
	auto a = new(buf) flexStruct(100);
	cout << a->size << " " << hex << (long long)a << " " <<(long long )a->c <<" " <<  (long long)((long long)a->c - (long long)a )<<endl;
	cout <<&(a->aa)<<" " <<&(a->b) <<" "<<&(a->c) <<" " <<endl;
	cout << (long long)buf<<endl;
	cout << dec <<sizeof(flexStruct) << endl;
	return 0;
}



/* part19：模板参数推导 左右值 */
//template<class T>





/* 模板类下的万能引用 */

template<typename... Args>
class MyClass {
public:
    template<typename... T>
    void myFunction(T&&... args) {
        // 使用std::forward将参数转发给其他函数
        otherFunction(std::forward<T>(args)...);
    }

private:
    void otherFunction(Args&&... args) {// 右值
        // 在这里使用参数
//        cout << 
    }
        void otherFunction(Args&... args) {// 左值
        // 在这里使用参数
//        cout << 
    }
};

int main1111(void) {
	MyClass<int, double, std::string> obj;
	obj.myFunction(42, 3.14, "hello");
	int a = 1;
//	obj.myFunction(a, 3.14, "hello");//error
	
	
}


