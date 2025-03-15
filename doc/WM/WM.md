[返回主页](../../README.md)

> WM算法借鉴BM算法的坏字符的思想，并且适用于多模式串，唯一不同的是，WM使用的时坏字符块，并且使用了前缀来防止遍历过多具有相同后缀的模式串。参考[WM](https://blog.csdn.net/pi9nc/article/details/9124623)。

> 主要思想：对于不同长度的模式串，均只考虑最短的模式串那个长度。对尾部字符块，找最近的可以匹配上的相同串的位置，找不到可以全跳过去，找到了就对齐，对于尾部字符块能匹配上的，则继续看前缀（目的是防止存在过多相同尾字符块的模式串），前缀能对上则尝试全匹模式串。



```cpp
#include<iostream>
#include<string>
#include<vector> 
#include<unordered_map>

using namespace std; 

// https://blog.csdn.net/pi9nc/article/details/9124623 
class WM{
public:
	WM(vector<string> p):p(p){
		buildmap();
	}
	vector<pair<int,string>> search(const string& text){
		vector<pair<int,string>> ans;
		int len = text.size();
		for(int i=min_len-1;i<len;){
			cout<<i<<endl;
			auto suffix_str = text.substr(i-B+1,B);
			cout<<"suffix str:"<<suffix_str<<endl;
			auto prefix_str = text.substr(i-min_len+1,prefix_len);
			cout<<"prefix str:"<<prefix_str<<endl;
			auto s_hash = hash(suffix_str);
			auto p_hash = hash(prefix_str);
			if(shift.count(s_hash)==0){
				i+= (min_len-1 -(-1+B-1));
				continue;
			}
			if(shift[s_hash]){
				i+=shift[s_hash];
				continue;
			}
			//shift=0
			if(prefix[s_hash].count(p_hash)){
				for(auto& pp:prefix[s_hash][p_hash]){
					auto plen = pp.size();
					//[i-min_len+1,)  plen
					if(i-min_len+1+plen>len){
						continue;
					}
					if(text.substr(i-min_len+1,plen)==pp){
						ans.push_back({i-min_len+1,pp});
					}
				}
			}
			i++;
		}
		return ans; 
	}
private:
	void buildmap(){ // 构建shift、prefix、pattern表 
	// shift对应于后缀  prefix对应于前缀   shift=0时(即在min_len下的结尾子串) -> 多prefix  每个prefix对应多个pattern
		auto sz = p.size();
		min_len = p[0].size();
		for(auto& t:p)
			min_len = min(min_len,(int)t.size());
		// shift
		for(auto & t:p){
			string a = t.substr(0,min_len);
			for(int i=0;i<=min_len-B;++i){
				auto block = a.substr(i,B);
				auto hashval = hash(block);
				if(shift.count(hashval)==0){
					shift[hashval] = min_len-B+1;
				} 
				shift[hashval] = min(min_len - i - B,shift[hashval]); // 尾部index相减 
			}
		}
		// when shift=0, build prefix
		for(auto& t:p){
			auto suffix_str = t.substr(min_len-B,B);
			auto prefix_str = t.substr(0,prefix_len);
			auto s_hash = hash(suffix_str);
			auto p_hash = hash(prefix_str);
			prefix[s_hash][p_hash].push_back(t);
		}
		cout<<"build success"<<endl;	
	}
	uint32_t hash(string t){ // 自定义字符串的hash函数,不超过3字节,否则越界 
		uint32_t hash = 0;
		for(auto a:t)
			hash = (hash<<8)|((uint8_t)a);
		return hash; 
	}
	vector<string> p; // 多模式串 
	const int B =2; // 字符块大小
	const int prefix_len = 2; // prefix前缀表内前缀大小 
	int min_len; 
	unordered_map<uint32_t,int> shift;
	unordered_map<uint32_t,unordered_map<uint32_t,vector<string>>> prefix; // shift_suffix -> multi prefix_str -> multi pattern
};

int main(void)
{
	vector<string> p = {"abcde","bcbde","abcabe"};
	WM wm(p);
	auto ans = wm.search("dcbacabcde");
	if(ans.empty())cout<<"not found"<<endl;
	for(auto a:ans){
		cout<<a.first<<" "<<a.second<<endl;
	}
}
```
