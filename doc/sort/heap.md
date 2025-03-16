# 堆排

[返回主页](../../README.md)

```cpp
class HeapSort {
public:
    vector<int> sortArray(vector<int>& nums) {
        makeHeap(nums);
        sortHeap(nums);
        return nums;
    }
    void makeHeap(vector<int>& nums){
        int sz = nums.size();
        for(int i=sz/2-1;i>-1;i--)adjustHeap(nums,i,sz);
    }
    void adjustHeap(vector<int>& nums,int pre,int sz){
        int tmp = nums[pre];
        int child = pre*2+1;
        // cout<<"sz="<<sz<<endl;
        while(child<sz){
            // cout<<child<<endl;
            if(child+1<sz&&nums[child+1]>nums[child])child++;
            if(nums[child]>tmp){
                nums[pre] = nums[child];
                pre = child;
                child = pre*2+1;
            }else break;
        }
        nums[pre] = tmp;
    }
    void sortHeap(vector<int>& nums){
        int sz = nums.size();
        cout<<"sort heap"<<endl;
        for(int i=sz-1;i>-1;i--){
            cout<<"sort "<<i<<endl;
            cout<<"num_0="<<nums[0]<<endl;
            swap(nums[i],nums[0]);
            adjustHeap(nums,0,i);
        }
    }
};

```
