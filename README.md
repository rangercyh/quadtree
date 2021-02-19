# quadtree

代码来自：https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det

回答者在 visual FX 工作多年，写的很朴质。

主要的思想是尽量使用平坦数组来代替在tree里存储元素，tree只操作切分关系。对移除元素合并切分的操作延迟到每帧末尾只做一次，这样能够减少元素在同一帧移动过程中多次触发合并和重新切分的操作。
另外并没有使用常见的直接存储元素aabb的方式来用空间换时间，主要考虑是cpu cache的命中，在这种存aabb的情况下并不如在运行时计算的效果好，主要因为遍历的操作比较频繁，相较于缓存带来的优势，
由于aabb内存消耗破坏cpu cached的问题更大。

回答者后面还写了关于松散四叉树（非严格按照中心点切分）的实现思路跟实现效果，以及统一大小的节点grid分割以及松散\紧致grid分割方式。

主要考虑将来做碰撞计算的时候可能用得上，抽空导出lua接口。

发现有人已经写了unity的例子，不过我主要用在服务器计算，还是得写一遍c跟lua的：https://github.com/Appleguysnake/DragonSpace-Demo
