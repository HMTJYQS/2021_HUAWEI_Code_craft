# 2021_HUAWEI_Code_craft

![frame](./data/source_image/frame.png)

## 2021华为软件精英挑战赛代码与算法思路

能参加此次华为软件精英挑战赛并能进复赛，还是很感谢两位队友，由于都是机械专业，本来就是跨领域参赛，且第一次参加该类型的赛事，走了许多弯路，代码不知道写了多少个版本，但是真正能降低成本提高效率的不多，以后有机会再参加的话，对于版本优化一定要在充分考虑好的情况下再撸代码，否则完全是浪费时间，最后，感谢枭哥的灵魂调参，让我们队成功在最后三分钟前进了12名！最终获得了初赛29名的成绩。

## 赛题分析
已经有很多博主写过对本次比赛赛题的分析了，就简单谈一下个人的看法（仅个人看法，勿喷）。

一开始看到这个题目，立马就想到了背包问题的解法，同样是找到最低成本，同样是把物品放入背包里面，只不过这里是把虚拟机放入服务器里面，但是这个问题复杂一些，需要考虑服务器的两个节点，以及虚拟机的单双节点，但是比赛有个点让采用背包问题的解法有点困难，就是服务器内虚拟机存在删除情况，传统的背包问题并不需要考虑背包内的物品会减少，所以可以找到最优解，而存在删除情况的话，可能导致最开始的完美配置，在删除出现后效果并不好，且虚拟机的部署存在时间顺序，这些问题都造成了背包问题解法应用在这里的难度。

下面是我们队对于整个赛题的思路：

## 购买服务器
在每一天的最开始，我们会选择一个“当日最佳服务器”，它应该装得下所有虚拟机防止出现bug，且价格要低的同时核数内存数比值要尽量贴合待部署虚拟机的需求资源。综合来讲，应当满足以下条件：1.能够装下目前已知的最大的虚拟机；2.函数值最低；3.如果函数值相同，则相对价格最低者优先。

- 函数值 = 相对价格*价格权重+核内比匹配度 （价格权重用于调整更加重视价格还是匹配度）
- 相对价格公式=（购买价格+单日运行成本*剩余天数）/（核数+内存数*量纲权重）
- 核内比匹配度公式 = （剩余未部署虚拟机的总核数内存数比值-服务器核内比）/剩余未部署虚拟机的总核数内存数比值 +惩罚值（惩罚值初值为0，若核内比一个大于1一个小于1则惩罚值为100，从而进一步保证核内比的匹配度）
之后每当遇到无法部署虚拟机的情况则买一个"当日最佳服务器"，然后继续部署虚拟机


## 处理请求
处理请求时，我们使用一个函数值来衡量部署进某个服务器节点的行为的优劣，函数值越小则部署行为越优秀。然后遍历搜索固定数量（遍历数量我们取到了9000，说明基本可以遍历所有部署可能）的可行的部署行为，选择函数值最小的部署行为作为添加请求的最终部署方案。
- 函数值=目标服务器节点剩余核数+剩余内存数*量纲权重+惩罚值（从剩余资源数小的服务器开始部署能提高服务器利用率，惩罚值则会确保部署行为会尽量避开某些不合理的方案）
- 惩罚值：初值为0，若使用了空服务器则惩罚值+10000（说明鼓励用尽可能少的服务器来部署虚拟机，从而节约电费），若待部署虚拟机核数内存数差值和目标服务器节点剩余核数内存数差值乘积小于0则惩罚值+1000（说明这个虚拟机部署进去会拉大目标服务器的核数内存数差异，造成不均衡拉低利用效率）。
- 可能有用但并没有实装的惩罚值类型：只有双节点虚拟机的服务器部署进了单节点虚拟机，虚拟机删除时间比服务器中的最晚删除时间要晚，虚拟机删除时间比服务器中的最早删除时间要早
- 代码中有尝试每天依次选择”当日最佳服务器“排行榜前`server_choose_times`名的服务器作为扩容服务器进行处理请求并计算总花费值，然后选择总花费值（总购买价格+总运行成本*剩余天数）最低的方案。但是一直没有调试成功，server_choose_times只能取1，不然会报错。
## 迁移
最开始我们的迁移策略就是把最后面的服务器内虚拟机往前面迁移，尽可能把后面的服务器搬空，但是这样的效果并不是特别好，因为虽然题目上说迁移在每天部署的最开始，但是计算当日的运行成本还是在迁移之前，也就是说迁移的重点应该是让后续的部署可以更好，而不仅仅是把后面的服务器搬空。

### 迁移策略一
首先通过迁移将各服务器内的核数内存数进行均衡，具体做法如下：
- 排序：为了提高迁移的速度，先将服务器按照核数内存数差值排序，差值最大的在最前面，迁移从差值最大的服务器开始
- 迁移虚拟机选择：找到差值最大服务器后，找到该服务器内核数或内存数最大的虚拟机（根据该服务器是核数大还是内存大确定）
- 迁移：将选出的最大虚拟机迁移到其他服务器内（其实该迁移如果考虑迁移到哪个服务器，效果会更好，后来实在没想到有什么好的方法可以确定迁移到哪个服务器合适），就根据排序表逆向迁移到服务器内

### 迁移策略二
把均衡放在正式迁移前面就是考虑，可以把迁移也认为是一次部署，如果提前均衡好，对迁移的效果肯定有提升，具体思路如下：
- 排序：同样为了提高迁移的速度和效果，先将服务器按照核数内存数总剩余值排序，剩余值最大的在最前面，迁移从剩余值最大的服务器开始
- 迁移虚拟机选择：找到剩余值最大服务器后，将该服务器内所有的虚拟机全部尝试迁移出去
- 迁移：待迁移虚拟机尝试迁移到剩余值最小的服务器内，如果失败，继续尝试迁移到剩余值第二小的服务器，一直循环下去

# 总结
