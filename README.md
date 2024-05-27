# kfs

设计实现一个Linux内核模块,此模块完成如下功能

- [x] 将新创建的文件系统的操作接口和VFS对接.
- [x] 实现新的文件系统的超级块、dentry、inode的读写操作.
- [x] 实现新的文件系统的权限属性,不同的用户不同的操作属性.
- [x] 实现和用户态程序的对接,用户程序
- [x] 设计实现一个用户态应用程序,可以将一个块设备(可以用文件模拟)格式化成自己设计的文件系统的格式.
- [x] 设计一个用户态的测试用例应用程序,测试验证自己的文件系统的open/read/write/ls/cd 等通常文件系统的访问.

## 编译运行

安装依赖

```bash
sudo apt-get install fuse3 libfuse3-dev pkg-config
```

编译得到文件系统 `src/kfs` 和磁盘格式化程序 `mkfs/mkfs`

```bash
make
```

创建 `disk.img` 文件(1000 MiB), 并使用 `mkfs/mkfs` 将其格式化为 ext4 文件系统格式

```bash
make disk
```

挂载文件系统到 `tmp/` 下

```bash
mkdir tmp
make run
```

可以进入 `tmp/` 目录下进行操作, 例如创建/打开/读取/修改文件等等, 所有修改都会保存在 `disk.img` 中, 所有的操作日志保存在 `kfs.log` 中

结束操作后使用取消挂载文件系统即可

```bash
umount tmp
```

或者以调试模式挂载并执行

```bash
make debug_run
```

> 结束后使用 <kbd>ctrl</kbd> + <kbd>c</kbd> 退出调试模式

## TODO

- [ ] ext4 日志
- [ ] 更好的 inode/data bitmap 选择

## 文档

项目文档见 `md-docs/`, 在线阅读: [kfs document](https://luzhixing12345.github.io/kfs/)

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