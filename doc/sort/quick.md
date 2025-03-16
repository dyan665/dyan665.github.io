# 快排

[返回主页](../../README.md)

```cpp
class QuickSort {
public:
    vector<int> sortArray(vector<int>& nums) {
        qsort(nums,0,nums.size()-1);
        return nums;
    }
    void qsort(vector<int>& nums,int left,int right){
        int l=left,r=right;
        if(left>=right)return;
        swap(nums[left],nums[(left+right)/2]);
        int tar = nums[left];
        while(left<right){
            while(left<right&&nums[right]>=tar)right--;
            nums[left]=nums[right];
            while(left<right&&nums[left]<=tar)left++;
            nums[right]=nums[left];
        }
        nums[left]=tar;
        qsort(nums,l,left);
        qsort(nums,left+1,r);
    }
};

```
