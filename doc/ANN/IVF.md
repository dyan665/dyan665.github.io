# 聚类+倒排索引 IVF算法

## 原理
> 1. 基本原理：对于向量，首先基于k-means进行聚类，假设有nlist个质心，然后在查询阶段，首先计算出nprobe个离用户向量最近的质心，然后在其所在的聚类中进行暴力搜索，此方法在nprobe<<nlist时，大大降低复杂度；此外，IVF算法在聚类后，还能组合其它算法，比如PQ乘积量化，faiss中有IVF-Flat（仅聚类）、IVF-PQ（聚类+PQ）；
> 2. IVF-PQ算法：首先基于k-means聚类为nlist个类，在每个聚类中，再用全局PQ算法进行降维，比如对M个子向量，每个子向量聚类为ksub个类，搜索时，先找在nlist中找nprobe个最近的聚类（基于原始向量维度计算），然后在聚类中，暴力算出每个子向量下到ksub个质心的距离组成表，对聚类中的每个向量，基于PQ算法算出的降维后的向量查表得到对应的近似距离，求和就约为其向量间的最终距离，最后只要按距离排序取top即可；
> 3. 复杂度：比如总数为n的向量，每个向量维度为d，训练为nlist个质心的聚类，计算全部向量到质心的距离复杂度为O(n*d*nlist)，更新质心位置O(n*d)，假如聚类迭代次数为niter，则总复杂度为O(n*d*nlist*niter)，向量分配到对应质心O(n*d*nlist)，构建倒排索引O(n)，因此总复杂度为O(n*d*nlist*niter)；查询复杂度O(d⋅(nlist+nprobe⋅(n/nlist)));

## k-means原理
> 对于n个点，维度为d，假如聚类为k个类，首先随机生成k个维度为d的质心，然后对n个点，计算到每个质心的距离，然后选取最近的质心作为其中心点（选取所属的聚类），然后更新质心位置（为该聚类所有点的位置均值），然后进行下一步迭代直到收敛。假如迭代次数为iter，则总复杂度为O(n*d*k*iter)。

[返回主页](../../README.md)
