# 并查集

[返回主页](../../README.md)

# 原理
> 要点：map记录子到父的关系，在查找根的find函数中，顺便修改子节点的根为0层的根，方便后续查找。
> 注意：对于新的节点，若未记录在map中，则需要先创建新的连通。

```cpp
class Union {
public:
    int cnt = 0;
    // 找连通的根节点，根节点相同则表明两点是属于一个连通里的
    int find(unordered_map<int,int>& pre,int child){
        if(!pre.count(child))pre[child]=child; // 新的点，创建新连通
        if(pre[child]!=child)pre[child] = find(pre,pre[child]); // 将根的一层往下的子节点，迁移到根下为一层子节点，方便后续查找
        return pre[child]; // 返回根节点
    }

    // 合并连通，若某个节点是新的，顺便创建新连通然后再合并
    void myunion(unordered_map<int,int>& pre,int child,int parent){
        if(!pre.count(child))cnt++; // 新节点
        if(find(pre,child)!=find(pre,parent))cnt--; // 两者不属于一个连通
        pre[find(pre,child)] = find(pre,parent); // 合并两个连通
    }
    // 岛屿连通数量
    int numIslands(vector<vector<char>>& grid) {
        int row = grid.size();
        if(!row)return 0;
        int col = grid[0].size();
        if(!col)return 0;
        unordered_map<int,int> pre; // child -> father
        for(int i=0;i<row;i++){
            for(int j=0;j<col;j++){
                if(grid[i][j]=='0')continue;
                bool flag=true;//是否左边或者上边有岛屿
                //zuo
                if(j&&grid[i][j-1]=='1'){
                    flag = false;
                    myunion(pre,i*col+j,i*col+j-1); // 合并连通
                }
                //shang
                if(i&&grid[i-1][j]=='1'){
                    flag = false;
                    myunion(pre,i*col+j,i*col+j-col);// 合并连通
                }                    
                if(flag)myunion(pre,i*col+j,i*col+j); // 附近不存在连通的点，新建一个连通
            }
        }
        return cnt;
    }
};

```
