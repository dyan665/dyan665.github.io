#include<iostream>
#include<functional>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>

using namespace std;

template<class T>
class SafeQueue{
	mutex _m;
	queue<T> _q;
	public:
		SafeQueue(SafeQueue& q){
			lock_guard<mutex> t(q._m);
			_q = q._q;
		}
		SafeQueue(SafeQueue&&q){
			lock_guard<mutex> t(q._m);
			_q = move(q);
		}
		bool enqueue(T& t){
			lock_guard<mutex> lck(_m);
			_q.emplace(t);
			return true;
		}
		
		bool enqueue(T&& t){
			lock_guard<mutex> lck(_m);
			_q.emplace(move(t));
			return true;
		}
		
		bool dequeue(T& t){
			lock_guard<mutex> lck(_m);
			if(_q.empty())return false;
			t = move(_q.front());
			_q.pop();
		}
		
		size_t size(){
			lock_guard<mutex> t(_m);
			return _q.size();
		}
};

class ThreadPool{
	class ThreadWorker{
		int id;
		ThreadPool *p;
		public:
			ThreadWorker(int id, ThreadPool* p):id(id),p(p){}
			
	};
	
	
	
	
	
	
	
	
	
	public:
};


