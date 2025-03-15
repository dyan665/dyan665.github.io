[返回主页](../../README.md)

> 主要思想：当失配时，针对已经匹配好的串，从以模式串开头找最长的子串与当前匹配好的串尾串相等，然后将串移动过去对齐，图可参考[KMP](http://www.ruanyifeng.com/blog/2013/05/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm.html)。

> 比如对于模式串ABCEFABCQ，对于待配串ABCEFABCWWWW，若在Q处失配，此时匹配好的串为ABCEFABC，此时就需要从整个模式串开头开始，找最长的与当前匹配好的串（ABCEFABC）的尾串相等的串，可以找到最长的串ABC，此时就能将模式串往后移动5位，让W前的ABC刚好对齐模式串的开头，就跳过了模式串开头的三位，达到了利用已匹配的信息跳过部分匹配的目的。

> KMP有一个缺点就是：对于模式串位0000，待配串000100010001，这时反复在最后一位失配的情况下，会导致性能恶化，为o(n*m)，更进一步的方法是BM算法。

> next记录的是，当前位置失配时，待配串中已经匹配的部分的尾串，和模式串从开头已经匹配的最大位置，当失配时可以跳转的继续尝试匹配的位置。next数组求解，其实就相当于模式串自己匹配自己，同时利用已经计算出的next来求解后面还未求的next。

```cpp
#include<vector>
#include<iostream>
#include<string>
// KMP算法在尾部反复失匹的情况下，o(m*n) BM在反复失匹时更优
using namespace std;

class KMP {
	public:
	KMP(string p):p(p){
		buildnext();
	}
	int search(const string& s){
		int ssz = s.size();
		int psz = p.size();
		int i=0,j=0;
		while(i<ssz){
			if(j==-1||s[i]==p[j]){ // 此处j被j==psz保证范围，i通过while保证范围 
				i++;
				j++;
			} else{
				j = next[j];//KMP算法匹配时是移动模式串index，s串不动；BM算法则是移动s串index，模式串始终从尾部开始匹
			}
			if(j==psz)return i-psz;
		}
		return -1;
	}

	private:
	void buildnext(){
		// code here
		int sz = p.size();
		next.resize(sz,0);
		next[0]=-1;//0位置失配则跳过，-1表示跳过
		int i=0,k=-1;// k为p_pattern的下标位置 -1表示无法匹配只能移动s_pattern了
		while(i<sz-1){ // 匹配逻辑类似于上面的search， 此处i需要小于sz-1，因为i++会导致越界
			if(k==-1||p[i]==p[k]){ // 此处k始终小于等于i，保证了i范围即可 
				i++;//匹配串下一个位置
				k++;//模式串最大匹配位置下一个位置
				next[i]=k;//位置i失配时，就跳转到k位置继续尝试，k之前的都已匹配上忽略
			}else {
				k=next[k];
			}
		}
		// code here
		for(auto t:next)cout<<t<<" ";
		cout << endl;
	}
	string p;
	vector<int> next;
};

int main(void) {
	string p = "exampleexam";
	string s = "this is a exampleexam";
	KMP kmp(p);
	cout << kmp.search(s) << endl;
}


```
