# 堆排

[返回主页](../../README.md)

# 原理
> 树用数组进行模拟，从最后一个拥有叶子节点的节点开始，依次往数组前进行调整，每次调整只对根节点向树下进行调整，一直到符合排序或者到叶子节点。
> 对全部数字排序时，将堆的根节点与数组最末尾的节点交换，然后去掉最末尾节点，重新调整堆即可，一直交换完全部节点，就全部有序。
> 获取第k大的位置，建立k个元素的小根堆，将新元素与堆的根对比，若新元素大就和堆的根交换重新调整即可。获取第k小反之亦反。

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
