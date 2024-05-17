# kfs

设计实现一个Linux内核模块,此模块完成如下功能

- [ ] 将新创建的文件系统的操作接口和VFS对接.
- [ ] 实现新的文件系统的超级块、dentry、inode的读写操作.
- [ ] 实现新的文件系统的权限属性,不同的用户不同的操作属性.
- [ ] 实现和用户态程序的对接,用户程序
- [ ] 设计实现一个用户态应用程序,可以将一个块设备(可以用文件模拟)格式化成自己设计的文件系统的格式.
- [ ] 设计一个用户态的测试用例应用程序,测试验证自己的文件系统的open/read/write/ls/cd 等通常文件系统的访问.

## 编译运行

安装依赖

```bash
sudo apt-get install fuse3 libfuse3-dev pkg-config
```

编译

```bash
make
```

创建 `test.img` 文件(1000 MiB) 并格式化为 ext4 格式

```bash
make disk
```

挂载文件系统(默认到 tmp/)目录下

```bash
mkdir tmp
make run
```

可以进入 tmp/ 目录下进行操作, 例如创建/打开/读取/修改文件等等, 最后所有的操作都会被保存在 test.img 中

## 文档

学习文件系统的评价指标以及测试、跑分方法, 需要测试:
1. 正确性
   - fstest [pjd-fstest The test suite checks POSIX compliance - 测试文件系统posix 接口兼容性](https://www.cnblogs.com/xuyaowen/p/pjd-fstest.html)
   - 这个工具很老,可能有坑
2. 一般性能指标
   使用IOZone即可
3. 与论文主题相关的特定评测指标(某项操作的吞吐量、延迟)
   一般需要自己实现,或者寻找有无相关工具有类似功能
   
   这一篇文章基本概括完了 [文件系统测试工具整理](https://www.cnblogs.com/xuyaowen/p/filesystem-test-suites.html)

学习编写用户态文件系统

1. FUFE 框架
   评价:性能较差,相关资料较多
   - [用户态文件系统框架FUSE的介绍及示例](https://zhuanlan.zhihu.com/p/59354174)
   - [吴锦华 / 明鑫 : 用户态文件系统 ( FUSE ) 框架分析和实战](https://cloud.tencent.com/developer/article/1006138)
   - [maastaar writing-a-simple-filesystem-using-fuse](https://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/)
   - [fuse detailed documentation](https://stackoverflow.com/questions/15604191/fuse-detailed-documentation)
   - [sshfs](https://github.com/libfuse/sshfs)
2. [拦截系统调用](https://github.com/pmem/syscall_intercept) + DirectIO操作裸磁盘
   性能较好,相关资料较少
3. 全用户态实现+用户态磁盘驱动:spdk框架
   性能最好,有完整框架

   [spdk articles](https://spdk.io/cn/articles/)

FAST,OSDI,SOSP,ASPLOS,USENIX ATC, EuroSys, SC,SoCC, HotOS, HotStorage, MSST,TC,TOS,TPDS

## 参考

- [Linux 文件系统(一):抽象](https://www.bilibili.com/video/BV1jM411W7jV)
- [动手实现一个文件系统,提高自己代码能力,加深对底层的理解,探索自己的就业新可能](https://www.bilibili.com/video/BV1eV411A7gw)
  - [好文分享:EXT文件系统机制原理详解](https://www.51cto.com/article/603104.html)
  - [Project 06: Simple File System](https://www3.nd.edu/~pbui/teaching/cse.30341.fa19/project06.html)
- [NTFS 是目前最先进的文件系统吗?](https://www.zhihu.com/question/20619659)
- [libfuse](https://github.com/libfuse/libfuse)
- [simplefs](https://github.com/sysprog21/simplefs)
- [扒一扒 Linux 文件系统的黑历史](https://zhuanlan.zhihu.com/p/28828826)
- [fuse-backend-rs](https://github.com/cloud-hypervisor/fuse-backend-rs)
- [300行代码带你实现一个Linux文件系统](https://zhuanlan.zhihu.com/p/579011810)
- [动手写一个简单的文件系统](https://www.jianshu.com/p/8966d121263b)
- [Assignment 11: File system in libfuse](https://course.ccs.neu.edu/cs3650sp22/a11.html)
- [文件系统](https://realwujing.github.io/linux/kernel/%E6%96%87%E4%BB%B6%E7%B3%BB%E7%BB%9F/)
- [星星说编程](https://space.bilibili.com/50657960/channel/series)