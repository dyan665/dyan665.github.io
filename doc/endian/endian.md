# 大小端

[返回主页](../../README.md)

```cpp
#include<iostream>
#include<functional>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
using namespace std;

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


int main(){
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
    cout << en->d1 <<" "<<en->d2<<" " <<en->d3 <<" " <<en->d4<<endl;//0 1 2 0    // 在0001 0010中，按d1/d2/d3/d4倒着排
    
    endian2 *en2 = (endian2*)&t;
    cout << en2->d1 <<" "<<en2->d2<<" " <<en2->d3 <<" " <<en2->d4<<endl;//78 4 5 2
    
    endian3 *en3 = (endian3*)pt;//78   在0111 1000中按d1/d2/d3/d4倒着排
    cout <<(unsigned) en3->d1 <<" "<<(unsigned)en3->d2<<" " <<(unsigned)en3->d3 <<" " <<(unsigned)en3->d4<<endl;//0 0 7 1
    /*
        结论：大小端针对的字节序，对于字节内还是按照从高位到低位的排列顺序
        对于结构体位域，则按大小端排列即可。
        比如，首先看大小端（比如上述为小端），然后将uint32_t在排列好(按照读出的顺序)，然后看结构体，按小端倒着排，有几位占几位，然后按正序看值。比如endian3中d4，占两位，对于内存中就是占最高两位，也就是01，所以d4的值就是1
        重点：首先得到读出顺序，找到对应字节，结构体位域始终从前往后，小端就是在读出顺序字节中倒着排，大端就正着排
    */
    return 0;
}
```
