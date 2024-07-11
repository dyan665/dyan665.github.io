# 有限状态机

[返回主页](../../REAMME.md)

## 有限状态机的特征
- 全部处于的状态是有限的
- 任意时刻，只会处于其中某一种状态
- 在某些事件下，会促使状态机从一种状态转变为另一种状态

## 有限状态机的要素
- 现态：当前所处状态
- 次态：当条件满足后，即将转移的下一个状态
- 动作：当满足某个事件时执行的动作；动作执行完毕后可以转移到另一个状态或保持原有状态
- 条件：转移状态所需的条件，当满足条件时，会触发一个动作或进行状态转移

## 伪代码
```cpp
/* part18：有限状态机 */

typedef enum {
	STATUS_1 = 0,
	STATUS_2,
	STATUS_3,
	STATUS_4,
	STATUS_NONE
}STATUS;

typedef enum {
	EVENT_1 = 0,
	EVENT_2,
	EVENT_3,
	EVENT_4,
	EVENT_NONE
}EVENT;

typedef void(*CALLBACK)(void* session/*save all status*/,void* input);

typedef struct{
	bool isValid;
	STATUS status;
	EVENT event;
	CALLBACK callbackfunc;
}fsmStruct;

fsmStruct fsm[STATUS_NONE][EVENT_NONE] = {};

bool register(STATUS s, EVENT e, CALLBACK callback){
	//check s and e and callback
	fsm[s][e] = {1,s,e,callback};
	return true;
}

void callbackexample(void* session, void* input){//STATUS_1 when get EVENT 1
	//read session
	//process input and update session
	//change STATUS and save it in session
	return;
}

void dispatch_event(session, msg, eventtype) {
	auto callback = fsm(session->status, eventtype);
	callback(session, msg);
}

int main(void) {
	//for loop
	while(true){
		//socket wait for event
		//epoll()
		//when rcv a msg, check msg type, according to now status, get event type
		getsession(msg);//get seesion
		eventtype = geteventtype(session, msg);
		dispatch_event(session, msg, eventtype);
	}
	return 0;
}
```