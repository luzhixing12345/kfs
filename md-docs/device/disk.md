
# disk

毫不夸张的说, 硬盘堪称机械工程的一个奇迹!

![disks](https://raw.githubusercontent.com/learner-lu/picbed/master/disks.gif)

> 一个非常好的硬盘机械结构可视化的视频: [How do Hard Disk Drives Work?  💻💿🛠](https://www.youtube.com/watch?v=wtdnatmVdIg)

## 硬盘结构

![20240415171941](https://raw.githubusercontent.com/learner-lu/picbed/master/20240415171941.png)

硬盘包含很多个盘片, 每一个盘片有**两面**, 在逻辑上被划分为磁道、柱面以及扇区.

- 磁盘在格式化时被划分成许多**同心圆**,这些同心圆轨迹叫做**磁道**(Track),磁道从外向内从0开始顺序编号;
- 所有盘面上的同一磁道构成一个**圆柱**,通常称做**柱面**(Cylinder),每个圆柱上的磁头由上而下从"0"开始编号;
- 每个磁道会被分成许多段**圆弧**,每段圆弧叫做一个**扇区**,扇区从"1"开始编号,每个扇区中的数据作为一个单元同时读出或写入,操作系统以扇区(Sector)形式将信息存储在硬盘上,每个扇区包括**512**个字节的数据和一些其他信息

## 

为了读到硬盘的数据, 它有两个自由度

![20240327202619](https://raw.githubusercontent.com/learner-lu/picbed/master/20240327202619.png)

磁盘性能调优

## 参考

- [存储器系统](https://zhuanlan.zhihu.com/p/105388861)
- [不同磁道的扇区数是否相同?的回答](https://www.zhihu.com/question/20537787/answer/77591552)
- [硬盘结构(图文展示)](https://www.cnblogs.com/cyx-b/p/14095057.html)
- [存储设备原理:1-Bit 信息的存储 (磁盘、光盘;闪存和 SSD) [南京大学2023操作系统-P25] (蒋炎岩)](https://www.bilibili.com/video/BV1Bh4y1x7tv/)
- [Cylinder-head-sector](https://en.wikipedia.org/wiki/Cylinder-head-sector)