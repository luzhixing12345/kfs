
# performance

[Filesystem Testing Tools](https://ext4.wiki.kernel.org/index.php/Filesystem_Testing_Tools)

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