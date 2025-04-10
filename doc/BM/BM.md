
[返回主页](../../README.md)

> BM算法的主要思想：从尾部开始匹配（相较于从头开始，尾部开始能更早发现失配，能跳转更大距离，优化了KMP下的反复失配问题）、bad char（坏字符）、good suffix（好子串）。图参考[BM](http://ruanyifeng.com/blog/2013/05/boyer-moore_string_search_algorithm.html)。

> 注意：失配时，BM始终从模式串的尾部开始匹配。

> 坏字符：记录失配情况下，s串中失配字符对应到的模式串中字符的最近位置，直接对齐。

> 好子串：记录的是当失配时，与尾串的子串重合的串中靠后的位置（防止错过可能的匹配机会，比如存在一个靠头的串和中间的串，肯定选中间的串继续匹），对于已经匹配上的模式串中的尾串，可能相同的子串（可以是在模式串中间，也可以是在模式串开头），对于尾串的子串，只能对应模式串的开头位置，假如能对应模式串中间位置，若最后能成功匹配全部模式串，说明这个中间位置，其实可以和全部尾串对齐，也就和前面说的记录靠后的位置相悖了，所以尾串的子串，只能和模式串开头的串对齐。


```cpp
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

class BM {
	public:
		BM(string s) : p(s) {
			getBC();
			getGS();
		}
		void getBC() { // 坏字符表
			int sz = p.size();
			for(int i=0;i<256;i++) bc[i]=-1;
			for (int i = 0; i < sz; ++i) {
				bc[p[i]] = i;
			}
		}
		void getGS() { // 计算好后缀表
			int sz = p.size();
			gs.resize(sz, sz);
			vector<int> suffix(sz, 0);// 相同后缀长度
			for (int i = 0; i < sz; i++) {
				int pidx = i, eidx=sz-1;// 以i结尾  以sz-1结尾 重合后缀长度
				while (pidx > -1 && p[pidx] == p[eidx]){
					pidx--;
					eidx--;
				}
				suffix[i] = i - pidx;
			}
			// 1. 部分匹  即倒着匹配，当在某个位置失配时，其中存在子串能与开头开始的串重合，此时
      			//  必须从尾考虑，这样才能优先匹长的子串，失配时尽可能移动少，配合i只递增 
			for (int i = 0, j = sz - 1; j > -1; j--) {
				if (suffix[j] == j + 1) { // 始于首的串 [0,j] 和尾串能够全匹配上
					while (i < sz - 1 - j) // 倒着匹时0到sz-1-j时失配   [0,j]作为尾串的子串部分匹配
						gs[i++] = sz - 1 - j; // 此时前面出现的串尾部是j，i一直是递增的 
				}
			}

			// 2. 全匹中间  即尾串全匹上  这种优先于子串匹开头  因为全匹更靠后
			for (int i = 0; i < sz - 1; i++) { // 匹到的中间是以i结尾的串
				gs[sz - 1 - suffix[i]] = sz - 1 - i; // gs的值等于两个相等的后缀的最后的index相减
			}
		}
		int search(const string &txt) {
			int i = 0, m = p.size(), n = txt.size();
			while(i<=n-m){
				int k = m-1; // 模式串倒着匹配 
				while(txt[i+k] == p[k])
					if(--k<0)
						break;
				if(k<0)
					break;
				else{
					i += max(gs[k], k-bc[txt[i+k]]); // 注意，此处为k 
				}
			}
			if(i<=n-m) return i;
			else return -1;
		}
	private:
		string p;//partten
		vector<int> gs;//good suffix
		unordered_map<char, int> bc;//bad char
};

int main() {
	BM s("example");
	cout << s.search("this is a example") << endl;
	return 0;
}




```
