#include<iostream>
#include<functional>
#include<mutex>
#include<thread>
#include<condition_variable>

using namespace std;

thread::id mainid = this_thread::get_id();


int thread2()
{
	auto id = this_thread::get_id();
	if(id == mainid){
		cout<<"this is main thread " << id<<endl;
	}else{
		cout<<"this is main thread " << id<<endl;
	}
}

union testunion {
	int a;
	char b;
} aa;
//4 12 4
struct AAA{
	char t;//8
//	int tt;
	struct {
		char a1 : 4;//
		char a2 : 4;	//4
		int a3;//4
//		long long a4;//8
		char a5;
	}a;//16  默认对齐为8 也就是前面的char t按8对齐
	int b;//4
//	char q;//1
};//32  最大成员16 默认8对齐  也就按8对齐
struct DDD {
	char a;//8  默认对齐为8字节
	struct{//此时a则为4字节 也就是说a并不是按min(默认对齐字节，嵌套结构体大小)来对齐的  是按min(默认对齐字节，嵌套结构体对齐字节)来对齐的
		int a1;
		int a2;
	}b;//8
//	long long b;//8
//	char c;
};//16

#pragma pack(4)
struct BBB {
	char a;//4
//	struct{
//		int a1;
//		int a2;
//	}bb;//8
	long long b;//8
//	int b2;
//	char c;
};//12 

struct CCC{
	char t;//4
//	int tt;
	struct {
		char a1 : 4;//
		char a2 : 4;	//4
		int a3;//4
		long long a4;//8
		char a5;//4 最大成员8 默认4 最终大小按4对齐
	}a;//16
	int b;//4
//	char q;//1
};//24

#pragma pack()


//struct align 结构体内部的结构体对齐：内部结构体自身对齐，然后内部结构体作为整体，在外层继续对齐，当内部结构体大小大于默认对齐大小时，按默认对齐大小
//最终结构体对齐  按当前最大成员与默认对齐 取较小的进行对齐


int main(void) {
	aa.a = 0x12345678;
	char * ap = (char*)&aa;
	cout << hex <<(int)*ap << endl;
	cout << sizeof(aa) <<endl;

	cout<<"this is a test" <<endl;
	AAA ma1;
//	long test = (long)a1a1;
	cout <<dec<< "AAA size:"<<(int)(sizeof(AAA)) << endl;
//	cout << (char*)&ma1.t- (char*)&ma1 << endl;
//	cout << (char*)&ma1.a- (char*)&ma1.t << endl;
//	cout << (char*)&ma1.a.a3- (char*)&ma1.a << endl;
//	cout << (char*)&ma1.a.a4- (char*)&ma1.a.a3 << endl;
//	cout << (char*)&ma1.b - (char*)&ma1.a.a4 << endl;
	
	cout << "BBB size:"<<sizeof(BBB)<<endl;
	cout << "CCC size:"<<sizeof(CCC)<<endl;
	cout << "DDD size:"<<sizeof(DDD)<<endl;
	
	BBB testb;
	DDD testd;
//	DDD testd2 = testd;
	cout << (char*)&testb.b - (char*)&testb << endl;
	cout << (char*)&testd.b - (char*)&testd << endl;
	
	
	int ttta=0x1234;
	unsigned char* b=(unsigned char*)&ttta;
	printf("%X %X %X %X\n",b[0],b[1],b[2],b[3]);
	cout << hex << b[0]<<endl;
	cout << "test little " << hex <<b[0]<<" "<<b[1] <<" "<< b[2] << " " << b[3] <<endl;
	
	
	uint64_t t64 = 0x1234 ;
	struct{
		char a;
		int b;
	}tmp;
	
	
	
	
	return 0;
}