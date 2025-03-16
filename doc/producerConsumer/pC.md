# 生产者消费者模型

[返回主页](../../README.md)


## 原理
> 基于两个条件变量的同步，对于生产者，条件变量2阻塞于队列满并且在生产，当队列有空位或者停止生产时放通，当生产后，给条件变量1发通知（notify），唤醒一个消费者（防止消费者在全部阻塞）；
> 对于消费者，条件变量1阻塞于队列空并且在生产，当队列有产品时或者停止生产时放通，此时若无产品说明停止生产则退出，否则取出产品进行消费，消费完给条件变量2发通知，唤醒一个生产者。
> 当停止生产时，通过条件变量1和2，唤醒全部生产者和消费者。

```cpp

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
```

